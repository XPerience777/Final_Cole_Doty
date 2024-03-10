#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Final_meshes.h"
#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "CS330 Final Scene Project - Cole Doty"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Texture id
	GLuint gTextureIdWood;
	GLuint gTextureIdPlastic;
	GLuint gTextureIdWallpaper;
	GLuint gTextureIdMetal;
	GLuint gTextureIdLeather;
	GLuint gTextureIdKeys;
	// Shader program
	GLuint gProgramId;

	//Shape Meshes from Professor Brian
	Meshes meshes;

	// camera
	Camera gCamera(glm::vec3(0.0f, 3.0f, 10.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	bool isPerspective = true;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;

	//PI
	const double M_PI = 3.14159265358979323846f;
	const double M_PI_2 = 1.571428571428571;

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char*[], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId);
void UDestroyShaderProgram(GLuint programId);

////////////////////////////////////////////////////////////////////////////////////////
// SHADER CODE
/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 vertexPosition; // VAP position 0 for vertex position data
	layout(location = 1) in vec3 vertexNormal; // VAP position 1 for normals
	layout(location = 2) in vec2 textureCoordinate;

	out vec3 vertexFragmentNormal; // For outgoing normals to fragment shader
	out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
	out vec2 vertexTextureCoordinate;

	//Uniform / Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(vertexPosition, 1.0f); // Transforms vertices into clip coordinates

		vertexFragmentPos = vec3(model * vec4(vertexPosition, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

		vertexFragmentNormal = mat3(transpose(inverse(model))) * vertexNormal; // get normal vectors in world space only and exclude normal translation properties

		vertexTextureCoordinate = textureCoordinate;
	}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
	in vec3 vertexFragmentNormal; // For incoming normals
	in vec3 vertexFragmentPos; // For incoming fragment position
	in vec2 vertexTextureCoordinate;

	out vec4 fragmentColor; // For outgoing cube color to the GPU

	uniform vec4 objectColor;
	uniform vec3 ambientColor;
	uniform vec3 light1Color;
	uniform vec3 light1Position;
	uniform vec3 light2Color;
	uniform vec3 light2Position;
	uniform vec3 viewPosition;
	uniform sampler2D uTexture; // Useful when working with multiple textures
	uniform bool ubHasTexture;
	uniform float light1Strength = 0.1f; // Set ambient or global lighting strength
	uniform float light2Strength = 0.1f; // Set lighting strength
	uniform float ambientStrength = 0.1f; // Set lighting strength
	uniform float specularIntensity1 = 0.8f;
	uniform float highlightSize1 = 16.0f;
	uniform float specularIntensity2 = 0.8f;
	uniform float highlightSize2 = 16.0f;

	void main()
	{
		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

		//Calculate Ambient lighting
		vec3 ambient = ambientStrength * ambientColor; // Generate ambient light color

		//**Calculate Diffuse lighting**
		vec3 norm = normalize(vertexFragmentNormal); // Normalize vectors to 1 unit
		vec3 light1Direction = normalize(light1Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact1 = max(dot(norm, light1Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse1 = light1Strength * impact1 * light1Color; // Generate diffuse light color
		vec3 light2Direction = normalize(light2Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact2 = max(dot(norm, light2Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse2 = light2Strength * impact2 * light2Color; // Generate diffuse light color

		//**Calculate Specular lighting**
		vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
		vec3 reflectDir1 = reflect(-light1Direction, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent1 = pow(max(dot(viewDir, reflectDir1), 0.0), highlightSize1);
		vec3 specular1 = specularIntensity1 * specularComponent1 * light1Color;
		vec3 reflectDir2 = reflect(-light2Direction, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
		vec3 specular2 = specularIntensity2 * specularComponent2 * light2Color;

		//**Calculate phong result**
		//Texture holds the color to be used for all three components
		vec4 textureColor = texture(uTexture, vertexTextureCoordinate);
		vec3 phong1;
		vec3 phong2;

		if (ubHasTexture == true)
		{
			phong1 = (ambient + diffuse1 + specular1) * textureColor.xyz;
			phong2 = (ambient + diffuse2 + specular2) * textureColor.xyz;
		}
		else
		{
			phong1 = (ambient + diffuse1 + specular1) * objectColor.xyz;
			phong2 = (ambient + diffuse2 + specular2) * objectColor.xyz;
		}

		fragmentColor = vec4(phong1 + phong2, 1.0); // Send lighting results to GPU
	}
	);
///////////////////////////////////////////////////////////////////////////////////////


// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	//UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object
	meshes.CreateMeshes();

	// Create the shader program
	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
		return EXIT_FAILURE;

	// Load textures
	const char* texFilename = "../../resources/textures/wood.png";
	if (!UCreateTexture(texFilename, gTextureIdWood))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "../../resources/textures/plastic.png";
	if (!UCreateTexture(texFilename, gTextureIdPlastic))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "../../resources/textures/wallpaper.png";
	if (!UCreateTexture(texFilename, gTextureIdWallpaper))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "../../resources/textures/metal.jpg";
	if (!UCreateTexture(texFilename, gTextureIdMetal))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "../../resources/textures/leather.jpg";
	if (!UCreateTexture(texFilename, gTextureIdLeather))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "../../resources/textures/metalkeys.png";
	if (!UCreateTexture(texFilename, gTextureIdKeys))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}


	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdWood);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gTextureIdPlastic);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gTextureIdWallpaper);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gTextureIdMetal);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, gTextureIdLeather);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, gTextureIdKeys);

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{
		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}

	// Release mesh data
	//UDestroyMesh(gMesh);
	meshes.DestroyMeshes();

	// Release texture
	UDestroyTexture(gTextureIdWood);
	UDestroyTexture(gTextureIdPlastic);
	UDestroyTexture(gTextureIdWallpaper);
	UDestroyTexture(gTextureIdMetal);
	UDestroyTexture(gTextureIdLeather);
	UDestroyTexture(gTextureIdKeys);

	// Release shader program
	UDestroyShaderProgram(gProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
	static const float cameraSpeed = 2.5f;

	//Exit
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//Movment
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);

	//Veiw Change
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		isPerspective = !isPerspective;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}


// Functioned called to render a frame
void URender()
{
	GLint objectColorLoc;
	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint viewPosLoc;
	GLint ambStrLoc;
	GLint ambColLoc;
	GLint light1ColLoc;
	GLint light1PosLoc;
	GLint light2ColLoc;
	GLint light2PosLoc;
	GLint light1StrLoc;
	GLint light2StrLoc;
	GLint objColLoc;
	GLint specInt1Loc;
	GLint highlghtSz1Loc;
	GLint specInt2Loc;
	GLint highlghtSz2Loc;
	GLint uHasTextureLoc;
	bool ubHasTextureVal;
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 rotation1;
	glm::mat4 rotation2;
	glm::mat4 translation;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Transforms the camera: move the camera back (z axis)
	view = gCamera.GetViewMatrix();

	// Creates a orthographic projection
	projection;
	// NOW check if 'isPerspective' and THEN set the perspective
	if (isPerspective) {
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else {
		float scale = 90;
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}

	// Set the shader to be used
	glUseProgram(gProgramId);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId, "model");
	viewLoc = glGetUniformLocation(gProgramId, "view");
	projLoc = glGetUniformLocation(gProgramId, "projection");
	viewPosLoc = glGetUniformLocation(gProgramId, "viewPosition");
	ambStrLoc = glGetUniformLocation(gProgramId, "ambientStrength");
	ambColLoc = glGetUniformLocation(gProgramId, "ambientColor");
	light1ColLoc = glGetUniformLocation(gProgramId, "light1Color");
	light1PosLoc = glGetUniformLocation(gProgramId, "light1Position");
	light2ColLoc = glGetUniformLocation(gProgramId, "light2Color");
	light2PosLoc = glGetUniformLocation(gProgramId, "light2Position");
	light1StrLoc = glGetUniformLocation(gProgramId, "light1Strength");
	light2StrLoc = glGetUniformLocation(gProgramId, "light2Strength");
	objColLoc = glGetUniformLocation(gProgramId, "objectColor");
	specInt1Loc = glGetUniformLocation(gProgramId, "specularIntensity1");
	highlghtSz1Loc = glGetUniformLocation(gProgramId, "highlightSize1");
	specInt2Loc = glGetUniformLocation(gProgramId, "specularIntensity2");
	highlghtSz2Loc = glGetUniformLocation(gProgramId, "highlightSize2");
	uHasTextureLoc = glGetUniformLocation(gProgramId, "ubHasTexture");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 1.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.2f, 0.2f, 0.2f);
	glUniform3f(light1ColLoc, 0.2f, 0.2f, 0.2f);
	glUniform3f(light1PosLoc, 3.0f, 2.0f, -2.0f);
	glUniform3f(light2ColLoc, 0.2f, 0.2f, 0.2f);
	glUniform3f(light2PosLoc, 0.0f, 4.0f, 0.0f);
	glUniform1f(light1StrLoc, 0.5f); //lighting strength set to 10%
	glUniform1f(light2StrLoc, 1.0f); //lighting strength set to 100%
	//set specular intensity
	glUniform1f(specInt1Loc, 5.0f);
	glUniform1f(specInt2Loc, 10.0f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 3.0f);
	glUniform1f(highlghtSz2Loc, 12.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	//START OF PC MONITER MODEL GROUP
	//DESK
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(10.0f, 1.0f, 6.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, -0.005f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gPlaneMesh.nVertices);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//MONITER
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(8.0f, 4.5f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 5.0f, -2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.17f, 0.7f, 0.8f, 0.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);


	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//NECK
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(1.0f, 4.5f, 0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 2.5f, -2.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.75f, 0.0f, 0.5f, 0.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdWood);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//BASE
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(2.0f, 0.2f, 2.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.0f, -1.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.17f, 0.9f, 0.0f, 0.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);


	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//SCREEN
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(3.68f, 0.0f, 2.07f));
	// 2. Rotate the object
	rotation = glm::rotate(1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 5.0f, -1.49f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 1.0f, 0.0f, 0.0f, 1.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);


	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//START OF SPEAKER MODEL GROUP
	//SPEAKER BODY
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(6.0f, 1.0f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.785398f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-5.0f, 0.5f, -3.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.17f, 0.7f, 0.8f, 0.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);


	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//SPEAKER PLATE
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(2.95f, 0.45f, 0.45f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	rotation1 = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.646f, 0.5f, -2.646f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * rotation1 * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 4);


	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gPlaneMesh.nVertices);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//DIAL PLATE
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.5f, 1.0f, 0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(45.0f), glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-5.0f, 1.01f, -3.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 3);



	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gPlaneMesh.nVertices);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//KNOB
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.3f, 0.10f, 0.3f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-5.0f, 1.01f, -3.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.17f, 0.9f, 0.0f, 0.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 3);



	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//START OF KEYBOARD MODEL GROUP
	//KEYBOARD BODY
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(4.0f, 0.2f, 2.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-2.0f, 0.1f, 2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.17f, 0.7f, 0.8f, 0.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);


	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	//KEYBOARD FACE PLATE
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(2.0f, 2.0f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));	
	// 3. Position the object
	translation = glm::translate(glm::vec3(-2.0f, 0.201f, 2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 3);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gPlaneMesh.nVertices);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//KEYS
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(2.0f, 2.0f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-2.0f, 0.202f, 2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 5);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gPlaneMesh.nVertices);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//KEYBOARD WRIST REST BODY
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(4.0f, 0.2f, 0.8f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-2.0f, 0.1f, 3.6f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.17f, 0.7f, 0.8f, 0.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 4);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	//START OF MOUSE MODEL GROUP
	//MOUSE BODY
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gSphereMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.6f, 0.5f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(3.0f, 0.0f, 2.0));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.0f, 1.0f, 0.0f, 1.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//SCROLL WHEEL
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.2f, 0.1f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(3.0f, 0.4f, 1.7f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.0f, 0.0f, 1.0f, 1.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 3);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//MOUSEPAD
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(10.0f, 0.04f, 5.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.02f, 3.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objColLoc, 0.17f, 0.7f, 0.8f, 0.0f);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 4);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);



	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}


void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}