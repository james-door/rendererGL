# import build.EGL.glrendererEGL as renderer

# renderer.renderTriangle()

from build.EGL.glrendererEGL import GlRenderer
import numpy as np
import time 


renderer = GlRenderer(2000, 2000)
renderer.renderTriangle()