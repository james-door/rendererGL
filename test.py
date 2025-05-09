import numpy as np
import taichi as ti
import time
from ..build.EGL.glrendererEGL import GlRenderer

ti.init(arch=ti.cpu)

dim, n_grid, steps, dt = 3, 64, 25, 2e-4
n_particles = n_grid**dim // 2 ** (dim - 1)
dx = 1 / n_grid 

p_rho = 1
p_vol = (dx * 0.5) ** 2
p_mass = p_vol * p_rho
GRAVITY = [0, -9.8, 0]
bound = 3
E = 1000  # Young's modulus
nu = 0.2  #  Poisson's ratio
mu_0, lambda_0 = E / (2 * (1 + nu)), E * nu / ((1 + nu) * (1 - 2 * nu))  # Lame parameters
F_x = ti.Vector.field(dim, float, n_particles)
F_v = ti.Vector.field(dim, float, n_particles)
F_C = ti.Matrix.field(dim, dim, float, n_particles)
F_dg = ti.Matrix.field(3, 3, dtype=float, shape=n_particles)  # deformation gradient
F_Jp = ti.field(float, n_particles)

F_colors = ti.Vector.field(4, float, n_particles)
F_colors_random = ti.Vector.field(4, float, n_particles)
F_materials = ti.field(int, n_particles)
F_grid_v = ti.Vector.field(dim, float, (n_grid,) * dim)
F_grid_m = ti.field(float, (n_grid,) * dim)
F_used = ti.field(int, n_particles)
neighbour = (3,) * dim
WATER = 0
JELLY = 1
SNOW = 2


@ti.kernel
def substep(g_x: float, g_y: float, g_z: float):
    for I in ti.grouped(F_grid_m):
        F_grid_v[I] = ti.zero(F_grid_v[I])
        F_grid_m[I] = 0
    ti.loop_config(block_dim=n_grid)
    for p in F_x:
        if F_used[p] == 0:
            continue
        Xp = F_x[p] / dx
        base = int(Xp - 0.5)
        fx = Xp - base
        w = [0.5 * (1.5 - fx) ** 2, 0.75 - (fx - 1) ** 2, 0.5 * (fx - 0.5) ** 2]

        F_dg[p] = (ti.Matrix.identity(float, 3) + dt * F_C[p]) @ F_dg[p]  # deformation gradient update
        # Hardening coefficient: snow gets harder when compressed
        h = ti.exp(10 * (1.0 - F_Jp[p]))
        if F_materials[p] == JELLY:  # jelly, make it softer
            h = 0.3
        mu, la = mu_0 * h, lambda_0 * h
        if F_materials[p] == WATER:  # liquid
            mu = 0.0

        U, sig, V = ti.svd(F_dg[p])
        J = 1.0
        for d in ti.static(range(3)):
            new_sig = sig[d, d]
            if F_materials[p] == SNOW:  # Snow
                new_sig = ti.min(ti.max(sig[d, d], 1 - 2.5e-2), 1 + 4.5e-3)  # Plasticity
            F_Jp[p] *= sig[d, d] / new_sig
            sig[d, d] = new_sig
            J *= new_sig
        if F_materials[p] == WATER:
            # Reset deformation gradient to avoid numerical instability
            new_F = ti.Matrix.identity(float, 3)
            new_F[0, 0] = J
            F_dg[p] = new_F
        elif F_materials[p] == SNOW:
            # Reconstruct elastic deformation gradient after plasticity
            F_dg[p] = U @ sig @ V.transpose()
        stress = 2 * mu * (F_dg[p] - U @ V.transpose()) @ F_dg[p].transpose() + ti.Matrix.identity(
            float, 3
        ) * la * J * (J - 1)
        stress = (-dt * p_vol * 4) * stress / dx**2
        affine = stress + p_mass * F_C[p]

        for offset in ti.static(ti.grouped(ti.ndrange(*neighbour))):
            dpos = (offset - fx) * dx
            weight = 1.0
            for i in ti.static(range(dim)):
                weight *= w[offset[i]][i]
            F_grid_v[base + offset] += weight * (p_mass * F_v[p] + affine @ dpos)
            F_grid_m[base + offset] += weight * p_mass
    for I in ti.grouped(F_grid_m):
        if F_grid_m[I] > 0:
            F_grid_v[I] /= F_grid_m[I]
        F_grid_v[I] += dt * ti.Vector([g_x, g_y, g_z])
        cond = (I < bound) & (F_grid_v[I] < 0) | (I > n_grid - bound) & (F_grid_v[I] > 0)
        F_grid_v[I] = ti.select(cond, 0, F_grid_v[I])
    ti.loop_config(block_dim=n_grid)
    for p in F_x:
        if F_used[p] == 0:
            continue
        Xp = F_x[p] / dx
        base = int(Xp - 0.5)
        fx = Xp - base
        w = [0.5 * (1.5 - fx) ** 2, 0.75 - (fx - 1) ** 2, 0.5 * (fx - 0.5) ** 2]
        new_v = ti.zero(F_v[p])
        new_C = ti.zero(F_C[p])
        for offset in ti.static(ti.grouped(ti.ndrange(*neighbour))):
            dpos = (offset - fx) * dx
            weight = 1.0
            for i in ti.static(range(dim)):
                weight *= w[offset[i]][i]
            g_v = F_grid_v[base + offset]
            new_v += weight * g_v
            new_C += 4 * weight * g_v.outer_product(dpos) / dx**2
        F_v[p] = new_v
        F_x[p] += dt * F_v[p]
        F_C[p] = new_C


class CubeVolume:
    def __init__(self, minimum, size, material):
        self.minimum = minimum
        self.size = size
        self.volume = self.size.x * self.size.y * self.size.z
        self.material = material


@ti.kernel
def init_cube_vol(
    first_par: int,
    last_par: int,
    x_begin: float,
    y_begin: float,
    z_begin: float,
    x_size: float,
    y_size: float,
    z_size: float,
    material: int,
):
    for i in range(first_par, last_par):
        F_x[i] = ti.Vector([ti.random() for i in range(dim)]) * ti.Vector([x_size, y_size, z_size]) + ti.Vector(
            [x_begin, y_begin, z_begin]
        )
        F_Jp[i] = 1
        F_dg[i] = ti.Matrix([[1, 0, 0], [0, 1, 0], [0, 0, 1]])
        F_v[i] = ti.Vector([0.0, 0.0, 0.0])
        F_materials[i] = material
        F_colors_random[i] = ti.Vector([ti.random(), ti.random(), ti.random(), ti.random()])
        F_used[i] = 1


@ti.kernel
def set_all_unused():
    for p in F_used:
        F_used[p] = 0
        # basically throw them away so they aren't rendered
        F_x[p] = ti.Vector([533799.0, 533799.0, 533799.0])
        F_Jp[p] = 1
        F_dg[p] = ti.Matrix([[1, 0, 0], [0, 1, 0], [0, 0, 1]])
        F_C[p] = ti.Matrix([[0, 0, 0], [0, 0, 0], [0, 0, 0]])
        F_v[p] = ti.Vector([0.0, 0.0, 0.0])


def init_vols(vols):
    set_all_unused()
    total_vol = 0
    for v in vols:
        total_vol += v.volume

    next_p = 0
    for i, v in enumerate(vols):
        v = vols[i]
        if isinstance(v, CubeVolume):
            par_count = int(v.volume / total_vol * n_particles)
            if i == len(vols) - 1:  # this is the last volume, so use all remaining particles
                par_count = n_particles - next_p
            init_cube_vol(next_p, next_p + par_count, *v.minimum, *v.size, v.material)
            next_p += par_count
        else:
            raise Exception("???")


@ti.kernel
def set_color_by_material(mat_color: ti.types.ndarray()):
    for i in range(n_particles):
        mat = F_materials[i]
        F_colors[i] = ti.Vector([mat_color[mat, 0], mat_color[mat, 1], mat_color[mat, 2], mat_color[mat, 3]])


print("Loading presets...this might take a minute")

presets = [
    [
        CubeVolume(ti.Vector([0.55, 0.05, 0.55]), ti.Vector([0.4, 0.4, 0.4]), WATER),
    ],
    [
        CubeVolume(ti.Vector([0.05, 0.05, 0.05]), ti.Vector([0.3, 0.4, 0.3]), WATER),
        CubeVolume(ti.Vector([0.65, 0.05, 0.65]), ti.Vector([0.3, 0.4, 0.3]), WATER),
    ],
    [
        CubeVolume(ti.Vector([0.6, 0.05, 0.6]), ti.Vector([0.25, 0.25, 0.25]), WATER),
        CubeVolume(ti.Vector([0.35, 0.35, 0.35]), ti.Vector([0.25, 0.25, 0.25]), SNOW),
        CubeVolume(ti.Vector([0.05, 0.6, 0.05]), ti.Vector([0.25, 0.25, 0.25]), JELLY),
    ],
]
preset_names = [
    "Single Dam Break",
    "Double Dam Break",
    "Water Snow Jelly",
]

curr_preset_id = 2

paused = False

use_random_colors = False
particles_radius = 0.02

material_colors = [(0.1, 0.6, 0.9, 0.2), (0.93, 0.33, 0.23, 1.0), (1.0, 1.0, 1.0, 1.0)]


def init():
    global paused
    init_vols(presets[curr_preset_id])




def render(window, camera, scene, canvas):
    camera.track_user_inputs(window, movement_speed=0.03, hold_key=ti.ui.RMB)
    scene.set_camera(camera)

    scene.ambient_light((0, 0, 0))

    colors_used = F_colors_random if use_random_colors else F_colors
    scene.particles(F_x, per_vertex_color=colors_used, radius=particles_radius)

    scene.point_light(pos=(0.5, 1.5, 0.5), color=(0.5, 0.5, 0.5))
    scene.point_light(pos=(0.5, 1.5, 1.5), color=(0.5, 0.5, 0.5))

    canvas.scene(scene)


def main():
    renderer = GlRenderer(2000, 2000)
    background_colour = np.array([0.0, 0.0, 0.0])
    renderer.setBackgroundColour(background_colour)
    pos = np.array([0.5, 0.5, -3.0])
    lookat = np.array([0.5, 0.5, 1.0])
    renderer.setCamera(pos, lookat)

    init()

    frame_id = 0
    material_colors[WATER] = material_colors[WATER]
    material_colors[SNOW] =  material_colors[SNOW]
    material_colors[JELLY] = material_colors[JELLY]
    set_color_by_material(np.array(material_colors, dtype=np.float32))
    max_frames = 250
    while frame_id < max_frames:

        if not paused:
            for _ in range(steps):
                substep(*GRAVITY)

        colors_used = F_colors_random if use_random_colors else F_colors
        F_x_np = F_x.to_numpy()
        
        frame_start = time.time()
        colors_used_np = colors_used.to_numpy()
        renderer.particles(F_x_np, colors_used_np, 0.01)
        renderer.saveImageRGB(f"{frame_id}.png")
        
        frame_end = time.time()
        print(f"Frame time: {frame_end - frame_start}s")


        frame_id +=1

if __name__ == "__main__":
    main()