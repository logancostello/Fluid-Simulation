#include <iostream>
#include <glad/glad.h>
#include <cmath>

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

using namespace std;
using namespace glm;

bool playing = false;

float bbWidth = 6.0f;
float bbHeight = 4.0;

vec3 gravity = vec3(0, -9.81, 0);

int numWaterDrops;
vector<WaterDrop> water;
float collisionDamping = 0.9;

class Application : public EventCallbacks {

public:

	WindowManager * windowManager = nullptr;

	std::shared_ptr<Program> prog;

	shared_ptr<Shape> drop;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
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
		if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			playing = true;
			render();
			playing = false;
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
		
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
 		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/sphere.obj").c_str());
		
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

    void drawWaterDrop(WaterDrop waterDrop, std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		M->pushMatrix();
			M->translate(waterDrop.position);
			M->scale(vec3(waterDrop.radius, waterDrop.radius, waterDrop.radius));
			setModel(prog, M);
			drop->draw(prog);
		M->popMatrix();
    }

	void render() {
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

		for (WaterDrop& waterDrop : water) {
			if (playing) {
				waterDrop.Update(gravity);
				waterDrop.ResolveOutOfBounds(bbWidth, bbHeight, collisionDamping);
			}
			drawWaterDrop(waterDrop, prog, Model);
		}

		prog->unbind();

		Projection->popMatrix();
		View->popMatrix();

	}
};

int main(int argc, char *argv[]) {
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc < 2) {
		cout << "Usage: ./fluid-simulation num-water-drops";
		return 0;
	} else {
		// Create grid of water drops for start of simulation
		numWaterDrops = atoi(argv[1]);

		if (numWaterDrops == 1) {
			water.push_back(WaterDrop(0, 0, 0, 0.1));
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

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}