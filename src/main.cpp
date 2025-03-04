#include <iostream>
#include <glad/glad.h>
#include <cmath>
#include <chrono>
#include <random>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "WaterDrop.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>


using namespace std;
using namespace glm;

std::chrono::high_resolution_clock::time_point lastFrameTime;

bool playing = false;
vector<float> densities;

float bbWidth = 6.0f;
float bbHeight = 4.0f;

vec3 gravity = vec3(0, 0, 0);

int numWaterDrops;
vector<WaterDrop> water;
float collisionDamping = 0.5;

float maxDensity = 0;
float targetDensity = 5.0f;
float pressureMultiplier = 20;

float kernelRadius = 0.7f;

vector<vec3> predictedPositions;

void setup() {
	water.clear();
	if (numWaterDrops == 1) {
		water.push_back(WaterDrop(0, 0, 0, 1));
	} else {
		float sqrtDrops = sqrt(numWaterDrops);
		float squareSize = ceil(sqrtDrops);
	
		for (int i = 0; i < squareSize; i++) {
			for (int j = 0; j < squareSize; j++) {
				float radius = 1 / sqrtDrops;
				float x = (2 - 2 / sqrtDrops) * ((i / (squareSize - 1)) - 0.5);
				float y = -(2 - 2 / sqrtDrops) * ((j / (squareSize - 1)) - 0.5);
	
				if (j * squareSize + i < numWaterDrops) {
					water.push_back(WaterDrop(x, y, 0, radius));
				} 
			}
		}
	}
}

void setupRandom() {
	water.clear();
	random_device rd;
  	mt19937 gen(rd());

	std::uniform_real_distribution<float> x_distrib(-bbWidth / 2, bbWidth / 2);
	std::uniform_real_distribution<float> y_distrib(-bbHeight / 2, bbHeight / 2);

	for (int i = 0; i < numWaterDrops; i++) {
		float x = x_distrib(gen);
		float y = y_distrib(gen);

		// float scale = 10 / (float)numWaterDrops;
		float scale = 0.04; // 1 / 25

		water.push_back(WaterDrop(x, y, 0, scale));
	}
}

glm::vec3 randomDirection() {
    glm::vec3 dir;
    do {
        dir = glm::vec3(
            glm::linearRand(-1.0f, 1.0f),
            glm::linearRand(-1.0f, 1.0f),
            0
        );
    } while (glm::length(dir) == 0.0f); // Avoid zero vector
    return glm::normalize(dir);
}

class Application : public EventCallbacks {

public:

	WindowManager * windowManager = nullptr;

	std::shared_ptr<Program> prog;

	shared_ptr<Shape> drop;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			playing = !playing;
		}
		if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
			playing = false;
			numWaterDrops += 1;
			setup();
		}
		if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
			playing = false;
			numWaterDrops -= 1;
			setup();
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			playing = true;
			render(0.016f); // 60 fps
			playing = false;
		}
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {
			setup();
			playing = false;
		}
		if (key == GLFW_KEY_T && action == GLFW_PRESS) {
			setupRandom();
			playing = false;
		}
		if (key == GLFW_KEY_P && action == GLFW_PRESS) {
			kernelRadius += 0.1;
			maxDensity = 0;
		}
		if (key == GLFW_KEY_L && action == GLFW_PRESS) {
			kernelRadius -= 0.1;
			kernelRadius = std::max(kernelRadius, 0.1f);
		}
		if (key == GLFW_KEY_O && action == GLFW_PRESS) {
			targetDensity += 1.0f;
		}
		if (key == GLFW_KEY_K && action == GLFW_PRESS) {
			targetDensity -= 1.0f;
		}
		if (key == GLFW_KEY_I && action == GLFW_PRESS) {
			pressureMultiplier += 1.0f;
		}
		if (key == GLFW_KEY_J && action == GLFW_PRESS) {
			pressureMultiplier -= 1.0f;
		}
		if (key == GLFW_KEY_U && action == GLFW_PRESS) {
			gravity += vec3(0, 1, 0);
		}
		if (key == GLFW_KEY_H && action == GLFW_PRESS) {
			gravity -= vec3(0, 1, 0);
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Density: " << calculateDensity(vec2(posX, posY)) << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height) {
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory) {
		GLSL::checkVersion();

		// Set background color to grey.
		glClearColor(.2f, .2f, .2f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("density");
		prog->addUniform("maxDensity");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
	}

	void resize_obj(std::vector<tinyobj::shape_t> &shapes){
		float minX, minY, minZ;
		float maxX, maxY, maxZ;
		float scaleX, scaleY, scaleZ;
		float shiftX, shiftY, shiftZ;
		float epsilon = 0.001;
	 
		minX = minY = minZ = 1.1754E+38F;
		maxX = maxY = maxZ = -1.1754E+38F;
	 
		//Go through all vertices to determine min and max of each dimension
		for (size_t i = 0; i < shapes.size(); i++) {
		   for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
			  if(shapes[i].mesh.positions[3*v+0] < minX) minX = shapes[i].mesh.positions[3*v+0];
			  if(shapes[i].mesh.positions[3*v+0] > maxX) maxX = shapes[i].mesh.positions[3*v+0];
	 
			  if(shapes[i].mesh.positions[3*v+1] < minY) minY = shapes[i].mesh.positions[3*v+1];
			  if(shapes[i].mesh.positions[3*v+1] > maxY) maxY = shapes[i].mesh.positions[3*v+1];
	 
			  if(shapes[i].mesh.positions[3*v+2] < minZ) minZ = shapes[i].mesh.positions[3*v+2];
			  if(shapes[i].mesh.positions[3*v+2] > maxZ) maxZ = shapes[i].mesh.positions[3*v+2];
		   }
		}
	 
		 //From min and max compute necessary scale and shift for each dimension
		float maxExtent, xExtent, yExtent, zExtent;
		xExtent = maxX-minX;
		yExtent = maxY-minY;
		zExtent = maxZ-minZ;
		if (xExtent >= yExtent && xExtent >= zExtent) {
		   maxExtent = xExtent;
		}
		if (yExtent >= xExtent && yExtent >= zExtent) {
		   maxExtent = yExtent;
		}
		if (zExtent >= xExtent && zExtent >= yExtent) {
		   maxExtent = zExtent;
		}
		scaleX = 2.0 /maxExtent;
		shiftX = minX + (xExtent/ 2.0);
		scaleY = 2.0 / maxExtent;
		shiftY = minY + (yExtent / 2.0);
		scaleZ = 2.0/ maxExtent;
		shiftZ = minZ + (zExtent)/2.0;
	 
		//Go through all verticies shift and scale them
		for (size_t i = 0; i < shapes.size(); i++) {
		   for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
			  shapes[i].mesh.positions[3*v+0] = (shapes[i].mesh.positions[3*v+0] - shiftX) * scaleX;
			  assert(shapes[i].mesh.positions[3*v+0] >= -1.0 - epsilon);
			  assert(shapes[i].mesh.positions[3*v+0] <= 1.0 + epsilon);
			  shapes[i].mesh.positions[3*v+1] = (shapes[i].mesh.positions[3*v+1] - shiftY) * scaleY;
			  assert(shapes[i].mesh.positions[3*v+1] >= -1.0 - epsilon);
			  assert(shapes[i].mesh.positions[3*v+1] <= 1.0 + epsilon);
			  shapes[i].mesh.positions[3*v+2] = (shapes[i].mesh.positions[3*v+2] - shiftZ) * scaleZ;
			  assert(shapes[i].mesh.positions[3*v+2] >= -1.0 - epsilon);
			  assert(shapes[i].mesh.positions[3*v+2] <= 1.0 + epsilon);
		   }
		}
	}

	void initGeom(const std::string& resourceDirectory) {
 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
		//load in the mesh and make the shape(s)
 		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/lowpolySphere.obj").c_str());
		
		if (!rc) {
			cerr << errStr << endl;
		} else {
			//for now all our shapes will not have textures - change in later labs
			resize_obj(TOshapes); 
			drop = make_shared<Shape>(false);
			drop->createShape(TOshapes[0]);
			drop->measure();
			drop->init();
		}
	}

	/* helper for sending top of the matrix strack to GPU */
	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    }

	void drawRectangle(float width, float height, std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack> M) {
		// Save the current matrix state
		M->pushMatrix();
		
		// Create vertices for the rectangle centered at origin
		GLfloat vertices[] = {
			-width/2, -height/2, 0.0f,  // Bottom-left  (0)
			 width/2, -height/2, 0.0f,  // Bottom-right (1)
			 width/2,  height/2, 0.0f,  // Top-right    (2)
			-width/2,  height/2, 0.0f   // Top-left     (3)
		};
		
		// Simple normal (all pointing in z-direction)
		GLfloat normals[] = {
			0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f
		};
		
		// Indices for drawing lines (edges only)
		GLuint indices[] = {
			0, 1,  // Bottom edge
			1, 2,  // Right edge
			2, 3,  // Top edge
			3, 0   // Left edge
		};
		
		// Create and bind vertex array object
		GLuint VAO, VBO_pos, VBO_nor, EBO;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO_pos);
		glGenBuffers(1, &VBO_nor);
		glGenBuffers(1, &EBO);
		
		glBindVertexArray(VAO);
		
		// Set up position attribute
		glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(prog->getAttribute("vertPos"));
		glVertexAttribPointer(prog->getAttribute("vertPos"), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
		
		// Set up normal attribute
		glBindBuffer(GL_ARRAY_BUFFER, VBO_nor);
		glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
		glEnableVertexAttribArray(prog->getAttribute("vertNor"));
		glVertexAttribPointer(prog->getAttribute("vertNor"), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
		
		// Set up element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		
		// Set model matrix for the rectangle
		setModel(prog, M);
		
		// Draw the rectangle edges only
		glBindVertexArray(VAO);
		glDrawElements(GL_LINES, 8, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		
		// Clean up
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO_pos);
		glDeleteBuffers(1, &VBO_nor);
		glDeleteBuffers(1, &EBO);
		
		// Restore the matrix state
		M->popMatrix();
	}

	void drawCircle(float radius, int segments, std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack> M) {
		// Save the current matrix state
		M->pushMatrix();
		
		// Calculate vertices for the circle
		std::vector<GLfloat> vertices;
		std::vector<GLuint> indices;
		
		// Center of the circle (0, 0, 0)
		vertices.push_back(0.0f);  // x
		vertices.push_back(0.0f);  // y
		vertices.push_back(0.0f);  // z
		
		// Create vertices around the circle
		for (int i = 0; i <= segments; ++i) {
			float angle = (i * 2.0f * M_PI) / segments;
			vertices.push_back(radius * cos(angle));  // x
			vertices.push_back(radius * sin(angle));  // y
			vertices.push_back(0.0f);  // z
		}
		
		// Create indices to form the lines of the circle
		for (int i = 1; i < segments; ++i) {
			indices.push_back(0);  // Center of the circle
			indices.push_back(i);  // Current vertex on the circle
		}
		// Close the circle by connecting the last vertex to the first one
		indices.push_back(0);
		indices.push_back(segments);
	
		// Simple normal (all pointing in z-direction)
		std::vector<GLfloat> normals(vertices.size(), 0.0f);
		for (int i = 0; i < vertices.size() / 3; ++i) {
			normals[i * 3 + 2] = 1.0f;  // Set z-component of the normal
		}
	
		// Create and bind vertex array object
		GLuint VAO, VBO_pos, VBO_nor, EBO;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO_pos);
		glGenBuffers(1, &VBO_nor);
		glGenBuffers(1, &EBO);
		
		glBindVertexArray(VAO);
		
		// Set up position attribute
		glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(prog->getAttribute("vertPos"));
		glVertexAttribPointer(prog->getAttribute("vertPos"), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
		
		// Set up normal attribute
		glBindBuffer(GL_ARRAY_BUFFER, VBO_nor);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * normals.size(), normals.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(prog->getAttribute("vertNor"));
		glVertexAttribPointer(prog->getAttribute("vertNor"), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
		
		// Set up element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);
		
		// Set model matrix for the circle
		setModel(prog, M);
		
		// Draw the circle (as lines from center to each vertex)
		glBindVertexArray(VAO);
		glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		
		// Clean up
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO_pos);
		glDeleteBuffers(1, &VBO_nor);
		glDeleteBuffers(1, &EBO);
		
		// Restore the matrix state
		M->popMatrix();
	}
	
	

    void drawWaterDrop(WaterDrop waterDrop, std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		M->pushMatrix();
			M->translate(waterDrop.position);
			M->scale(vec3(waterDrop.radius, waterDrop.radius, waterDrop.radius));
			setModel(prog, M);
			drop->draw(prog);
		M->popMatrix();
    }

	float densityToPressure(float density) {
		if (density < 0.0f) {
			return 0.0f;  // Return zero pressure for negative densities
		}
	
		float densityDifference = density - targetDensity;
		float pressure = densityDifference * pressureMultiplier;
		return pressure;
	} 

	float calculateSharedPressure(float density1, float density2) {
		return (densityToPressure(density1) + densityToPressure(density2)) / 2.0;
	}

	vec3 calculatePressureForce(int samplePointIndex) {
		vec3 pressureForce = vec3(0.0f, 0.0f, 0.0f);

		for (int i = 0; i < numWaterDrops; i++) {
			if (i == samplePointIndex) continue;

			vec3 difference = predictedPositions[i] - water[samplePointIndex].position;
			float distance = length(difference);

			vec3 direction;
			if (distance == 0) {
				direction = randomDirection(); //should make this random later
			} else {
				direction = difference / distance;
			}

			float slope = smoothingKernelDerivative(kernelRadius, distance);
			float density = densities[i];
			float mass = 1.0;
			float sharedPressure = calculateSharedPressure(density, densities[samplePointIndex]);
			pressureForce += sharedPressure * direction * slope * mass / density; 
			
		}

		return pressureForce;
	}

	float smoothingKernel(float kernelRadius, float distance) {
		if (distance >= kernelRadius) return 0;

		float volume = (3.1415 * pow(kernelRadius, 4)) / 6; 
		return (kernelRadius - distance) * (kernelRadius - distance) / volume;
		
	} 

	float smoothingKernelDerivative(float kernelRadius, float distance) {
		if (distance >= kernelRadius) return 0;

		float scale = 12 / (pow(kernelRadius, 4) * 3.1415);
		return (distance - kernelRadius) * scale;

	}	
	

	float calculateDensity(vec2 samplePoint) {
		float density = 0;
		float mass = 1;

		for (WaterDrop& drop : water) {
			float distance = length(vec2(drop.position.x, drop.position.y) - samplePoint);
			float influence = smoothingKernel(kernelRadius, distance);
			density += influence * mass;
		}

		return density;
	} 

	void render(float deltaTime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		// View is global translation along negative z for now
		View->pushMatrix();
			Model->translate(vec3(0, 0, -5));

		// Draw base Hierarchical person
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));

		drawRectangle(bbWidth, bbHeight, prog, Model);
		drawCircle(kernelRadius, 100, prog, Model);

		predictedPositions.clear();
		for (WaterDrop& drop : water) {
			predictedPositions.push_back(drop.position + drop.velocity * 1.0f / 120.0f);
		}

		densities.clear();
		maxDensity = 0;

		// Calculate the densities per drop
		for (int i = 0; i < predictedPositions.size(); i++) {
			float currentDensity = calculateDensity(vec2(predictedPositions[i].x, predictedPositions[i].y));
			densities.push_back(currentDensity);
			maxDensity = std::max(currentDensity, maxDensity);
		}

		glUniform1f(prog->getUniform("maxDensity"), maxDensity);

		for (int i = 0; i < water.size(); i++) {
			glUniform1f(prog->getUniform("density"), densities[i]);
			if (playing) {
				vec3 pressure = calculatePressureForce(i);
				water[i].Update(pressure / densities[i], deltaTime);
				water[i].Update(gravity, deltaTime);
				water[i].velocity *= 0.995;
				water[i].ResolveOutOfBounds(bbWidth, bbHeight, collisionDamping);
			}
			
			drawWaterDrop(water[i], prog, Model);
		}
		

		prog->unbind();

		Projection->popMatrix();
		View->popMatrix();

	}
};

float getDeltaTime() {
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> deltaTime = currentFrameTime - lastFrameTime;
    lastFrameTime = currentFrameTime;

    return deltaTime.count();
}


int main(int argc, char *argv[]) {
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc < 2) {
		cout << "Usage: ./fluid-simulation num-water-drops";
		return 0;
	} else {
		// Create grid of water drops for start of simulation
		numWaterDrops = atoi(argv[1]);
		// setup();
		setupRandom();
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(1280, 960);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	lastFrameTime = std::chrono::high_resolution_clock::now();

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{ 
		cout << "=================" << endl;
		cout << "Target Density: " << targetDensity << endl;
		cout << "Kernel Radius: " << kernelRadius << endl;
		cout << "Pressure Multiplier: " << pressureMultiplier << endl;
		// float deltaTime = getDeltaTime();
		float deltaTime = 1.0f / 240.0f;

		// Render scene.
		application->render(deltaTime);

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}