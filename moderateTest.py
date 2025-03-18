from build.EGL.glrendererEGL import GlRenderer
import numpy as np
import time 


def main():


    renderer = GlRenderer(2000, 2000)
    renderer.hardcodedTest()
    
    return 
    background_colour = np.array([0.0, 0.0, 0.0])
    renderer.setBackgroundColour(background_colour)



    pos = np.array([0.0, 0.0 , -1.0])
    lookat = np.array([0.0, 0.0, 0.0])
    renderer.setCamera(pos, lookat)


    n_points = 1000
    frame_id = 0
    max_frames = 1   
    while frame_id < max_frames:
        points = np.random.uniform(-1, 1, (n_points, 3))
        colours= np.random.uniform(0, 1, (n_points, 4))

        
        
        frame_start = time.time()
        renderer.particles(points, colours, 0.01)
        
        # test = renderer.getImageRGB()
        # imsave("test.png",test)
        renderer.saveImageRGB("test.png")
        
        frame_end = time.time()
        print(f"Frame time: {frame_end - frame_start}s")


        frame_id +=1
        
        
if __name__ == "__main__":
    main()