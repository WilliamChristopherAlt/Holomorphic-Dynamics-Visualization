#define NOMINMAX

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <ctime>

#include <OpenGL/shaderClass.h>
#include <OpenGL/VAO.h>
#include <OpenGL/VBO.h>
#include <OpenGL/FBO.h>
#include <OpenGL/textureClass.h>

#include <math/math_util.h>
#include <filesUtil/myFile.h>
#include <math/random.h>

#include <Mandelbrot/src/spline.h>

uint32_t timeSinceEpochMillisec() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

GLuint scrWidth = 100;
GLuint scrHeight = 100;
float aspectRatio;
float aspectRatioInv;
float initialAreaWidth;
float initialAreaHeight;
float areaWidth;
float areaHeight;

bool parameterSpace = true;
const float ZOOM_SENSITIVITY = 0.1f;
const float MAX_ITER_INCREASE_SENSITIVITY = 5.0f;
const float GRADIENT_SENSITIVITY_SENSITIVITY = 0.5f;

const int GRADIENT_SIZE = 1024;
float gradientSensitivity = 0.2f; // Must be in range 0 to 1

const std::vector<float> colorPositions = { 0.0, 0.16, 0.42, 0.6425, 0.8575 };

std::vector<glm::vec3> colors =
{
	glm::vec3(0,   7,   100) / 255.999f,
	glm::vec3(32,  107, 203) / 255.999f,
	glm::vec3(237, 255, 255) / 255.999f,
	glm::vec3(255, 170,   0) / 255.999f,
	glm::vec3(130,   2,  50) / 255.999f
};

// Try this at your own risks, the resulting gradient looks ugly most of the time
// unsigned int seed = timeSinceEpochMillisec();
// std::vector<glm::vec3> colors =
// {
// 	glm::vec3(random(seed), random(seed), random(seed)),
// 	glm::vec3(random(seed), random(seed), random(seed)),
// 	glm::vec3(random(seed), random(seed), random(seed)),
// 	glm::vec3(random(seed), random(seed), random(seed)),
// 	glm::vec3(random(seed), random(seed), random(seed))
// };

bool scrolled = false;
bool moused = false;
bool keyed = false;
int frameAccumulationCount = 0;

bool flipX = false;
bool flipY = false;

float lastTime = glfwGetTime();
float deltaTime = 1.0f;

double lastX = 0.0;
double lastY = 0.0;
bool firstMouse = true;

std::vector<glm::vec2> selectedPoints = {glm::vec2(0.0f), glm::vec2(0.0f)};
int selectedIndex = 0;
const int NUM_POINTS = 2;

glm::vec2 offset = glm::vec2(0.0f);
float maxIter = 2000.0f;
float totalYOffset = 0.0f;

GLfloat ndcVertices[] =
{
	// Top left
	-1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	// Bottom right
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f
};

void screenshot(GLFWwindow* window)
{
	Texture2D* screenTexPtr = static_cast<Texture2D*>(glfwGetWindowUserPointer(window));

	// Allocate memory to store pixel data
	GLubyte* pixels = new GLubyte[3 * scrWidth * scrHeight]{ 0 };
	float* pixelsCumulative = new float[3 * scrWidth * scrHeight]{ 0 };

	// Transfer texture to the FBO then the pixels	
	FBO FBO(*screenTexPtr);
	FBO.Bind();
	glReadPixels(0, 0, scrWidth, scrHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	FBO.Delete();

	// Flip the image vertically(OpenGL's origin is at the bottom-left, but most images start at the top-left)
	for (int y = 0; y < scrHeight / 2; y++)
		for (int x = 0; x < scrWidth * 3; x++)
			std::swap(pixels[y * scrWidth * 3 + x], pixels[(scrHeight - 1 - y) * scrWidth * 3 + x]);

	std::string path = getPath("\\Images\\", 1);
	std::ostringstream posX, posY, zoom;
	posX << std::fixed << std::setprecision(10) << offset.x;
	posY << std::fixed << std::setprecision(10) << offset.y;
	zoom << totalYOffset;
	path += "x " + posX.str() + " y " + posY.str() + " zoom - " + zoom.str() + ".png";
	stbi_write_png(path.c_str(), scrWidth, scrHeight, 3, pixels, scrWidth * 3);

	delete[] pixels;
	delete[] pixelsCumulative;

	glViewport(0, 0, scrWidth, scrHeight);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	float xoffset = xpos - lastX;
	float yoffset = ypos - lastY;
	lastX = xpos;
	lastY = ypos;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		offset.x -= xoffset * 2.0f * areaWidth / scrWidth;
		offset.y += yoffset * 2.0f * areaHeight / scrHeight;
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		selectedPoints[selectedIndex].x = offset.x + (xpos / scrWidth * 2.0f - 1.0f) * areaWidth;
		selectedPoints[selectedIndex].y = offset.y - (ypos / scrHeight * 2.0f - 1.0f) * areaHeight;
	}

	moused = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Get NDC coordinates
	float mouseX = (xpos / scrWidth) * 2.0f - 1.0f;
	float mouseY = 1.0f - (ypos / scrHeight) * 2.0f;

	float mouseXWorld = offset.x + mouseX * areaWidth;
	float mouseYWorld = offset.y + mouseY * areaHeight;

	// Apply zoom
	totalYOffset += yoffset;
	areaWidth = exp(-ZOOM_SENSITIVITY * totalYOffset);
	areaHeight = areaWidth * aspectRatioInv;

	// Adjust offset so that the mouse position stays the same
	offset.x = mouseXWorld - mouseX * areaWidth;
	offset.y = mouseYWorld - mouseY * areaHeight;

	scrolled = true;
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
		keyed = true;
        switch (key)
        {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
                return;
            case GLFW_KEY_W:
            {
                int sign0 = mods == GLFW_MOD_SHIFT ? -1 : 1;
                maxIter += sign0 * MAX_ITER_INCREASE_SENSITIVITY;
                if (maxIter < 0)
                    maxIter = 0;
                break;
            }
            case GLFW_KEY_Z:
            {
                int sign1 = mods == GLFW_MOD_SHIFT ? -1 : 1;
                float zoom = exp(-ZOOM_SENSITIVITY * totalYOffset);
                gradientSensitivity += sign1 * GRADIENT_SENSITIVITY_SENSITIVITY * sqrt(zoom) * deltaTime;
                gradientSensitivity = clamp(gradientSensitivity, 0.0f, 1.0f);
                break;
            }
            case GLFW_KEY_R:
            {
                selectedPoints[selectedIndex] *= 0.0f;
                break;
            }
            case GLFW_KEY_X:
            {
                flipX = !flipX;
                break;
            }
            case GLFW_KEY_Y:
            {
                flipY = !flipY;
                break;
            }
            case GLFW_KEY_P:
            {
                parameterSpace = !parameterSpace;
                break;
            }
			case GLFW_KEY_S:
            {
                screenshot(window);
                break;
            }
			case GLFW_KEY_0:
			case GLFW_KEY_1:
			case GLFW_KEY_2:
			case GLFW_KEY_3:
			case GLFW_KEY_4:
			{
				if (mods & GLFW_MOD_CONTROL)
					selectedIndex = std::min(key - GLFW_KEY_0, NUM_POINTS - 1);
			}		
            default:
                break;
        }    
	}
}


template <typename T>
std::vector<glm::vec3> generateGradient(const std::vector<glm::vec3>& colors, const std::vector<float>& positions, int gradientSize)
{
	T spline = T(positions, colors);
	std::vector<glm::vec3> gradient(gradientSize);
	
	float t = 0;
	float dt = 1.0f / float(gradientSize - 1);

	for (int i = 0; i < gradientSize; i++)
	{
		gradient[i] = spline.interpolate(t);
		t += dt;
	}

	return gradient;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

	scrWidth = videoMode->width;
	scrHeight = videoMode->height;
	aspectRatio = (float) scrWidth / scrHeight;
	aspectRatioInv = (float) scrHeight / scrWidth;
	initialAreaWidth = 2.0f;
	initialAreaHeight = initialAreaWidth * aspectRatioInv;
	areaWidth = initialAreaWidth;
	areaHeight = initialAreaHeight;

	GLFWwindow* window = glfwCreateWindow(scrWidth, scrHeight, "Mandelbrot", primaryMonitor, NULL);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	
	// Introduce window to current context
	glfwMakeContextCurrent(window);
	gladLoadGL();

	glViewport(0, 0, scrWidth, scrHeight);

	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, keyboardCallback);

	std::vector<glm::vec3> gradient = generateGradient<MonotonicHermiteCyclic<glm::vec3>>(colors, colorPositions, GRADIENT_SIZE);

	std::string path = getPath("\\Shaders", 1);
	Shader drawShader(path + "\\vert.glsl", path + "\\frag.glsl");
	ComputeShader computeShader(path + "\\compute.glsl");

	drawShader.Activate();
	drawShader.setInt("tex", 0);

	Texture2D screenTex(scrWidth, scrHeight, GL_TEXTURE0);
    glBindImageTexture(0, screenTex.ID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glfwSetWindowUserPointer(window, &screenTex);

	computeShader.setBool("parameterSpace", parameterSpace);
	computeShader.setVec3Array("gradient", gradient.data(), GRADIENT_SIZE);
	computeShader.setInt("gradientSize", GRADIENT_SIZE);

	VAO VAO;
	VAO.Bind();
	VBOSimple VBO(ndcVertices, sizeof(ndcVertices));

	VAO.LinkAttrib(VBO, 0, 3, GL_FLOAT, 3 * sizeof(GLfloat), (void*)0);

	VAO.Unbind();
	VBO.Unbind();

	while (!glfwWindowShouldClose(window))
	{
		float currentTime = glfwGetTime();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		std::ostringstream fps;
		fps << std::fixed << std::setprecision(2) << 1.0f / deltaTime;
		std::ostringstream maxIterStr;
		maxIterStr << std::fixed << std::setprecision(2) << maxIter;
		std::string title = "Mandelbrot - Max Iteration:" + maxIterStr.str()  + " - FPS:" + fps.str();
		glfwSetWindowTitle(window, title.c_str());

		glClear(GL_COLOR_BUFFER_BIT);

		bool frameAccumulation;
		frameAccumulationCount++;

		if (moused || scrolled || keyed)
		{
			frameAccumulation = false;
			frameAccumulationCount = 1;
		}
		else	
			frameAccumulation = true;

		computeShader.Activate();

		computeShader.setBool("frameAccumulation", frameAccumulation);
		computeShader.setInt("frameAccumulationCount", frameAccumulationCount);
		computeShader.setBool("parameterSpace", parameterSpace);
		computeShader.setVec2Array("selectedPoints", selectedPoints.data(), NUM_POINTS);
		computeShader.setBool("flipX", flipX);
		computeShader.setBool("flipY", flipY);
		computeShader.setVec2("offset", offset);
		computeShader.setFloat("areaWidth", areaWidth);
		computeShader.setFloat("areaHeight", areaHeight);
		computeShader.setInt("maxIter", static_cast<int>(maxIter));
		computeShader.setFloat("gradientSensitivity", gradientSensitivity);
		computeShader.setFloat("pixelLength", areaWidth / scrWidth);

		computeShader.Activate();
		glDispatchCompute(ceil(scrWidth) / 8, ceil(scrHeight / 4), 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		drawShader.Activate();
		screenTex.SetActive();
		screenTex.Bind();
		drawShader.setInt("screen", 0);
		VAO.Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);

		scrolled = false;
		keyed = false;
		moused = false;
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	VAO.Delete();
	VBO.Delete();
	computeShader.Delete();
	drawShader.Delete();
	screenTex.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}