// InteractiveGraphics.cpp : Defines the entry point for the console application.
//

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cyTriMesh.h>
#include <cyGL.h>
#include <cyMatrix.h>
#include <lodepng.h>
#include <iostream>
#include <string>
#include <math.h>
#include <glm_includes.h>

struct {
	float r, g, b, a;
}color;

struct VertexData{
	cyPoint3f vertexPosition;
	cyPoint3f vertexNormal;
	cyPoint2f uv;
};

struct Material {
	cyPoint3f ambient;
	cyPoint3f diffuse;
	cyPoint3f specular;
	float shininess;
};

//Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float ASPECT_RATIO = WINDOW_WIDTH / (float)WINDOW_HEIGHT;
const float FOV = 0.785398f;
const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 1000.0f;
const float DEG2RAD = 0.0174533f;

//Teapot model variables
cyTriMesh teapotMesh;
GLuint teapotVAO, teapotVBO, teapotDiffuseTextureId, teapotSpecularTextureId;
cyGLSLShader teapotVertexShader, teapotFragmentShader;
cyGLSLProgram teapotShaderProgram;
Material teapotMaterial;
glm::vec3 teapotPosition(0.0f, 0.0f, 0.0f);

//Camera variables
glm::vec3 cameraPosition(0.0f, 0.0f, 70.0f);
bool isProjection = true;
float xCameraRot = 0.0f;
float yCameraRot = 0.0f;
glm::mat4 projection = glm::perspective(FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE);

//Light variables
cyTriMesh lightMesh;
GLuint lightVAO, lightVBO;
cyGLSLShader lightVertexShader, lightFragmentShader;
cyGLSLProgram lightShaderProgram;
glm::vec3 lightPosition(0, 8.0f, 30.0f);
bool isLightRotating;
float xLightRot = 0.0f;
float yLightRot = 0.0f;

//Plane variables
GLuint planeVAO, planeVBO, planeDiffuseTextureId;
cyGLSLShader planeVertexShader, planeFragmentShader;
cyGLSLProgram planeShaderProgram;
glm::vec3 planePosition(0.0f, -8.0f, 0.0f);
Material planeMaterial;
bool isPlaneMoving;
float xPlaneRot = 0.0f;
float yPlaneRot = 0.0f;

//General variables
float lastRightMousePos;
float lastLeftMousePosX;
float lastLeftMousePosY;
float rotationSpeed = 0.0174533f;
float xRot = 0.0f;
float yRot = 0.0f;
int pressedButton;

//Depth buffer
cyGLRenderDepth2D depthBuffer;

//Shadow map shader
cyGLSLShader shadowMapVertexShader, shadowMapFragmentShader;
cyGLSLProgram shadowMapShaderProgram;

//Callback functions
void CompileShaders();
void Display();
void Idle();
void GetKeyboardInput(unsigned char key, int xmouse, int ymouse);
void GetKeySpecial(int key, int xmouse, int ymouse);
void GetKeyUpSpecial(int key, int xmouse, int ymouse);
void GetMouseInput(int button, int state, int xmouse, int ymouse);
void GetMousePosition(int x, int y);
void TogglePerspective();

int main(int argc, char *argv[])
{	
	color = { 0.0f,0.0f,0.0f,0.0f };

	//Initialize window and opengl params
	{
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
		glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
		glutInitContextVersion(3, 3);
		glutInitContextProfile(GLUT_CORE_PROFILE);
		glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
		glutCreateWindow("Test");
		glutDisplayFunc(Display);
		glutIdleFunc(Idle);
		glutKeyboardFunc(GetKeyboardInput);
		glutMouseFunc(GetMouseInput);
		glutMotionFunc(GetMousePosition);
		glutSpecialFunc(GetKeySpecial);
		glutSpecialUpFunc(GetKeyUpSpecial);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glewInit();
	}
		
	char* filePath;
	if (argc > 1) {		
		filePath = argv[1];
		std::cout << "Loading file " << filePath << std::endl;
	}
	else {
		filePath = "Assets/Models/teapot/teapot.obj";
	}
	std::string parentFolder(filePath);
	parentFolder = parentFolder.substr(0, parentFolder.find_last_of("/") + 1);
	
	//Load primary mesh
	if (teapotMesh.LoadFromFileObj(filePath)) {
		
		//Teapot setup start
		{
			teapotMesh.ComputeBoundingBox();
			//Adjust model to accommodate bounding box at 0,0,0
			cyPoint3f temp = (teapotMesh.GetBoundMax() + teapotMesh.GetBoundMin()) / 2.0f;
			teapotPosition = -glm::vec3(temp.x, temp.y, temp.z);
			teapotMesh.ComputeNormals();

			bool hasTextureVertices = teapotMesh.HasTextureVertices();

			//Extract all vertex data
			VertexData* vertices = new VertexData[teapotMesh.NF() * 3];
			for (unsigned int i = 0; i < teapotMesh.NF(); i++) {
				
				unsigned int vertexIndex = i * 3;
				
				cyTriMesh::TriFace face = teapotMesh.F(i);
				vertices[vertexIndex].vertexPosition = teapotMesh.V(face.v[0]);
				vertices[vertexIndex + 1].vertexPosition = teapotMesh.V(face.v[1]);
				vertices[vertexIndex + 2].vertexPosition = teapotMesh.V(face.v[2]);

				cyTriMesh::TriFace normalFace = teapotMesh.FN(i);
				vertices[vertexIndex].vertexNormal = teapotMesh.VN(normalFace.v[0]);
				vertices[vertexIndex + 1].vertexNormal = teapotMesh.VN(normalFace.v[1]);
				vertices[vertexIndex + 2].vertexNormal = teapotMesh.VN(normalFace.v[2]);

				if (hasTextureVertices) {
					cyTriMesh::TriFace textureFace = teapotMesh.FT(i);
					vertices[vertexIndex].uv = cyPoint2f(teapotMesh.VT(textureFace.v[0]));
					vertices[vertexIndex + 1].uv = cyPoint2f(teapotMesh.VT(textureFace.v[1]));
					vertices[vertexIndex + 2].uv = cyPoint2f(teapotMesh.VT(textureFace.v[2]));
				}
			}

			glGenVertexArrays(1, &teapotVAO);
			glGenBuffers(1, &teapotVBO);
			glBindVertexArray(teapotVAO);
			glBindBuffer(GL_ARRAY_BUFFER, teapotVBO);
			glBufferData(GL_ARRAY_BUFFER, teapotMesh.NF() * 3 * sizeof(VertexData), vertices, GL_STATIC_DRAW);
			// Position vertex attribute
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexPosition));
			// Position vertex attribute
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexNormal));
			//UV
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, uv));		
			
			delete[] vertices;

			//Get material information
			if (teapotMesh.NM() > 0) {
				cyTriMesh::Mtl mat = teapotMesh.M(0);
				std::vector<unsigned char> imageBuffer;
				std::string diffuseMap = parentFolder + std::string(mat.map_Kd.data);
				unsigned width, height;
				int result = lodepng::decode(imageBuffer, width, height, diffuseMap, LodePNGColorType::LCT_RGB);
				if (result == 0) {
					glGenTextures(1, &teapotDiffuseTextureId);
					glBindTexture(GL_TEXTURE_2D, teapotDiffuseTextureId);

					// Set texture wrapping params
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

					// Set texture filtering params
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBuffer.data());
					glGenerateMipmap(GL_TEXTURE_2D);
				}

				imageBuffer.clear();
				std::string specularMap = parentFolder + std::string(mat.map_Ks.data);
				result = lodepng::decode(imageBuffer, width, height, specularMap, LodePNGColorType::LCT_RGB);
				if (result == 0) {
					glGenTextures(1, &teapotSpecularTextureId);
					glBindTexture(GL_TEXTURE_2D, teapotSpecularTextureId);

					// Set texture wrapping params
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

					// Set texture filtering params
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBuffer.data());
					glGenerateMipmap(GL_TEXTURE_2D);
				}
			}

			//Initialize material properties
			teapotMaterial.diffuse = cyPoint3f(1.0f, 0.0f, 0.0f);
			teapotMaterial.ambient = teapotMaterial.diffuse;
			teapotMaterial.specular = cyPoint3f(1.0f, 1.0f, 1.0f);
			teapotMaterial.shininess = 50.0f;
		}

		//Setup plane
		{
			const float planeHalfWidth = 50.0f;
			const float planeHalfHeight = 50.0f;
			VertexData* vertices = new VertexData[6];
			vertices[0].vertexPosition = cyPoint3f(-planeHalfWidth, 0.0f, -planeHalfHeight);
			vertices[0].uv = cyPoint2f(0.0f, 0.0f);
			vertices[0].vertexNormal = cyPoint3f(0.0f, 1.0f, 0.0f);
			vertices[1].vertexPosition = cyPoint3f(planeHalfWidth, 0.0f, -planeHalfHeight);
			vertices[1].uv = cyPoint2f(1.0f, 0.0f);
			vertices[1].vertexNormal = cyPoint3f(0.0f, 1.0f, 0.0f);
			vertices[2].vertexPosition = cyPoint3f(-planeHalfWidth, 0.0f, planeHalfHeight);
			vertices[2].uv = cyPoint2f(0.0f, 1.0f);
			vertices[2].vertexNormal = cyPoint3f(0.0f, 1.0f, 0.0f);
			vertices[3].vertexPosition = cyPoint3f(planeHalfWidth, 0.0f, -planeHalfHeight);
			vertices[3].uv = cyPoint2f(1.0f, 0.0f);
			vertices[3].vertexNormal = cyPoint3f(0.0f, 1.0f, 0.0f);
			vertices[4].vertexPosition = cyPoint3f(planeHalfWidth, 0.0f, planeHalfHeight);
			vertices[4].uv = cyPoint2f(1.0f, 1.0f);
			vertices[4].vertexNormal = cyPoint3f(0.0f, 1.0f, 0.0f);
			vertices[5].vertexPosition = cyPoint3f(-planeHalfWidth, 0.0f, planeHalfHeight);
			vertices[5].uv = cyPoint2f(0.0f, 1.0f);
			vertices[5].vertexNormal = cyPoint3f(0.0f, 1.0f, 0.0f);

			glGenVertexArrays(1, &planeVAO);
			glGenBuffers(1, &planeVBO);
			glBindVertexArray(planeVAO);
			glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
			glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(VertexData), vertices, GL_STATIC_DRAW);
			// Position vertex attribute
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexPosition));
			// Position vertex attribute
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexNormal));
			//UV
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, uv));

			//Initialize material properties
			planeMaterial.diffuse = cyPoint3f(0.6f, 0.6f, 0.6f);
			planeMaterial.ambient = planeMaterial.diffuse;
			planeMaterial.specular = cyPoint3f(1.0f, 1.0f, 1.0f);
			planeMaterial.shininess = 50.0f;

			delete[] vertices;
		}

		//Light mesh
		{
			char* lightMeshFilePath = "Assets/Models/light/light.obj";
			if (lightMesh.LoadFromFileObj(lightMeshFilePath)) {
				//Extract all vertex data
				VertexData* vertices = new VertexData[lightMesh.NF() * 3];
				for (unsigned int i = 0; i < lightMesh.NF(); i++) {

					unsigned int vertexIndex = i * 3;

					cyTriMesh::TriFace face = lightMesh.F(i);
					vertices[vertexIndex].vertexPosition = lightMesh.V(face.v[0]);
					vertices[vertexIndex + 1].vertexPosition = lightMesh.V(face.v[1]);
					vertices[vertexIndex + 2].vertexPosition = lightMesh.V(face.v[2]);

					cyTriMesh::TriFace normalFace = lightMesh.FN(i);
					vertices[vertexIndex].vertexNormal = lightMesh.VN(normalFace.v[0]);
					vertices[vertexIndex + 1].vertexNormal = lightMesh.VN(normalFace.v[1]);
					vertices[vertexIndex + 2].vertexNormal = lightMesh.VN(normalFace.v[2]);

				}

				glGenVertexArrays(1, &lightVAO);
				glGenBuffers(1, &lightVBO);
				glBindVertexArray(lightVAO);
				glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
				glBufferData(GL_ARRAY_BUFFER, lightMesh.NF() * 3 * sizeof(VertexData), vertices, GL_STATIC_DRAW);
				// Position vertex attribute
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexPosition));
				// Position vertex attribute
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexNormal));
				//UV
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, uv));

				delete[] vertices;
			}
			else {
				assert(false + "Failed to load light mesh");
			}
		}

		//Initialize depth buffer
		{
			depthBuffer.Initialize(true, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2, GL_DEPTH_COMPONENT32);
			depthBuffer.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);
			depthBuffer.SetTextureWrappingMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		}

		CompileShaders();

		glutMainLoop();		

	}
	else {
		std::cerr << "File not found" << std::endl;
	}
}

void Display() {

	using namespace glm;	

	vec3 cameraUp(0.0f, 1.0f, 0.0f);
	mat4 cameraRotationMatrix = eulerAngleXYZ(xCameraRot, yCameraRot, 0.0f);
	vec3 localCameraPosition = vec3(vec4(cameraPosition, 1.0f) * cameraRotationMatrix);
	cameraUp = normalize(vec3(vec4(cameraUp, 0.0f) * cameraRotationMatrix));

	mat4 cameraView = lookAt(localCameraPosition, vec3(0.0f), cameraUp);	

	mat4 lightSpaceMatrix;

	//Draw the shadow scene
	{
		vec3 lightUp(0.0f, 1.0f, 0.0f);
		mat4 lightRotationMatrix = eulerAngleXYZ(xLightRot, yLightRot, 0.0f);
		vec3 localLightPosition = vec3(vec4(lightPosition, 1.0f) * lightRotationMatrix);
		lightUp = normalize(vec3(lightRotationMatrix * vec4(lightUp, 0.0f)));
		mat4 lightView = lookAt(localLightPosition, vec3(0.0), lightUp);
		
		float near_plane = 6.2f, far_plane = 700.0f;
		float projectionCubeSize = 50.0f;
		mat4 lightProjection = perspective(glm::radians(70.0f), ASPECT_RATIO, near_plane, far_plane);

		lightSpaceMatrix = lightProjection * lightView;
		
		depthBuffer.Bind();
		glClearColor(color.r, color.g, color.b, color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Draw model	
		{
			//Setup matrices for teapot
			mat4 teapotTranslationMatrix;
			teapotTranslationMatrix = translate(teapotTranslationMatrix, teapotPosition);

			//Set all shader params for teapot
			{
				shadowMapShaderProgram.Bind();
				shadowMapShaderProgram.SetUniformMatrix4("lightSpaceMatrix", value_ptr(lightSpaceMatrix));
				shadowMapShaderProgram.SetUniformMatrix4("model", value_ptr(teapotTranslationMatrix));				
			}

			//Draw teapot
			{
				glBindVertexArray(teapotVAO);
				glDrawArrays(GL_TRIANGLES, 0, teapotMesh.NF() * 3);
			}			
		}

		depthBuffer.Unbind();
	}

	//Draw normal scene
	{
		glClearColor(color.r, color.g, color.b, color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				
		mat4 lightRotationMatrix = eulerAngleXYZ(xLightRot, yLightRot, 0.0f);
		vec3 localLightPosition = vec3(vec4(lightPosition, 1.0f) * lightRotationMatrix);

		//Draw model	
		{
			//Setup matrices for teapot
			mat4 teapotTranslationMatrix;
			teapotTranslationMatrix = translate(teapotTranslationMatrix, teapotPosition);

			//Set all shader params for teapot
			{
				teapotShaderProgram.Bind();
				teapotShaderProgram.SetUniformMatrix4("model", value_ptr(teapotTranslationMatrix));
				teapotShaderProgram.SetUniformMatrix4("view", value_ptr(cameraView));
				teapotShaderProgram.SetUniformMatrix4("projection", value_ptr(projection));
				teapotShaderProgram.SetUniform("lightPosition", localLightPosition.x, localLightPosition.y, localLightPosition.z);
				teapotShaderProgram.SetUniform("cameraPosition", localCameraPosition.x, localCameraPosition.y, localCameraPosition.z);
				teapotShaderProgram.SetUniform("ambient", teapotMaterial.ambient);
				teapotShaderProgram.SetUniform("diffuse", teapotMaterial.diffuse);
				teapotShaderProgram.SetUniform("specular", teapotMaterial.specular);
				teapotShaderProgram.SetUniform("shininess", teapotMaterial.shininess);

				//Pass shadow params				
				teapotShaderProgram.SetUniformMatrix4("lightSpaceMatrix", value_ptr(lightSpaceMatrix));
			}

			//Bind depth buffer
			{
				depthBuffer.BindTexture();
			}

			//Draw teapot
			{
				glBindVertexArray(teapotVAO);
				glDrawArrays(GL_TRIANGLES, 0, teapotMesh.NF() * 3);
			}
		}

		//Draw plane
		{
			//Setup matrices for plane
			mat4 planeModelMatrix;
			planeModelMatrix = translate(planeModelMatrix, planePosition);

			//Set shader variables
			{
				planeShaderProgram.Bind();
				planeShaderProgram.SetUniformMatrix4("model", value_ptr(planeModelMatrix));
				planeShaderProgram.SetUniformMatrix4("view", value_ptr(cameraView));
				planeShaderProgram.SetUniformMatrix4("projection", value_ptr(projection));
				planeShaderProgram.SetUniform("lightPosition", localLightPosition.x, localLightPosition.y, localLightPosition.z);
				planeShaderProgram.SetUniform("cameraPosition", localCameraPosition.x, localCameraPosition.y, localCameraPosition.z);
				planeShaderProgram.SetUniform("ambient", planeMaterial.ambient);
				planeShaderProgram.SetUniform("diffuse", planeMaterial.diffuse);
				planeShaderProgram.SetUniform("specular", planeMaterial.specular);
				planeShaderProgram.SetUniform("shininess", planeMaterial.shininess);

				//Pass shadow params				
				planeShaderProgram.SetUniformMatrix4("lightSpaceMatrix", value_ptr(lightSpaceMatrix));
			}

			//Bind depth buffer
			{
				depthBuffer.BindTexture();
			}

			//Draw plane
			{
				glBindVertexArray(planeVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}

		//Draw light
		{
			//Light matrices
			mat4 localLightModelMatrix;
			localLightModelMatrix = translate(localLightModelMatrix, localLightPosition);			

			mat4 lightMVP = projection * cameraView * localLightModelMatrix;

			//Bind shader
			{
				lightShaderProgram.Bind();
				lightShaderProgram.SetUniformMatrix4("mvp", value_ptr(lightMVP));
			}

			//Draw mesh
			{
				glBindVertexArray(lightVAO);
				glDrawArrays(GL_TRIANGLES, 0, lightMesh.NF() * 3);
			}
		}
	}

	glutSwapBuffers();
}

void Idle() {
	glutPostRedisplay();
}

void GetKeyboardInput(unsigned char key, int xmouse, int ymouse) {
	if (key == 27) {
		glutLeaveMainLoop();
	}
	if (key == 112) {
		TogglePerspective();
	}
}

void GetKeySpecial(int key, int xmouse, int ymouse) {
	
	if (key == GLUT_KEY_F6) {
		CompileShaders();
	}

	if (key == GLUT_KEY_CTRL_L || key == GLUT_KEY_CTRL_R) {
		isLightRotating = true;
	}

	if (key == GLUT_KEY_ALT_L || key == GLUT_KEY_ALT_R) {
		isPlaneMoving = true;
	}
}

void GetKeyUpSpecial(int key, int xmouse, int ymouse) {

	if (key == GLUT_KEY_CTRL_L || key == GLUT_KEY_CTRL_R) {
		isLightRotating = false;
	}

	if (key == GLUT_KEY_ALT_L || key == GLUT_KEY_ALT_R) {
		isPlaneMoving = false;
	}
}

void GetMouseInput(int button, int state, int xmouse, int ymouse) {
	pressedButton = button;
	if (GLUT_LEFT_BUTTON == button) {
		if (state == GLUT_DOWN) {
			lastLeftMousePosX = static_cast<float>(xmouse);
			lastLeftMousePosY = static_cast<float>(ymouse);
		}
		else {
			lastLeftMousePosX = static_cast<float>(xmouse);
			lastLeftMousePosY = static_cast<float>(ymouse);
		}
	}
	else {
		if (state == GLUT_DOWN) {
			lastRightMousePos = static_cast<float>(ymouse);
		}
	}
}

void GetMousePosition(int x, int y) {
	if (pressedButton == GLUT_LEFT_BUTTON) {
		float xDiff = (x - lastLeftMousePosX) * rotationSpeed;
		lastLeftMousePosX = static_cast<float>(x);
		float yDiff = (y - lastLeftMousePosY) * rotationSpeed;
		lastLeftMousePosY = static_cast<float>(y);
		if (isLightRotating) {
			yLightRot += xDiff;
			xLightRot += yDiff;
		}
		else if (isPlaneMoving) {
			yPlaneRot += xDiff;
			xPlaneRot += yDiff;
		}else{			
			yRot += xDiff;
			xRot += yDiff;
			xCameraRot = xRot;
			yCameraRot = yRot;
		}
	}else {
		float diff = static_cast<float>(y) - lastRightMousePos;
		lastRightMousePos = static_cast<float>(y);
		cameraPosition.z -= diff;
	}
}

void CompileShaders() {
	teapotVertexShader.CompileFile("Assets/Shaders/Vertex/shadow.glsl", GL_VERTEX_SHADER);
	teapotFragmentShader.CompileFile("Assets/Shaders/Fragment/shadow.glsl", GL_FRAGMENT_SHADER);
	teapotShaderProgram.Build(&teapotVertexShader, &teapotFragmentShader);

	planeVertexShader.CompileFile("Assets/Shaders/Vertex/shadow.glsl", GL_VERTEX_SHADER);
	planeFragmentShader.CompileFile("Assets/Shaders/Fragment/shadow.glsl", GL_FRAGMENT_SHADER);
	planeShaderProgram.Build(&planeVertexShader, &planeFragmentShader);

	lightVertexShader.CompileFile("Assets/Shaders/Vertex/light.glsl", GL_VERTEX_SHADER);
	lightFragmentShader.CompileFile("Assets/Shaders/Fragment/light.glsl", GL_FRAGMENT_SHADER);
	lightShaderProgram.Build(&lightVertexShader, &lightFragmentShader);

	shadowMapVertexShader.CompileFile("Assets/Shaders/Vertex/shadowMap.glsl", GL_VERTEX_SHADER);
	shadowMapFragmentShader.CompileFile("Assets/Shaders/Fragment/shadowMap.glsl", GL_FRAGMENT_SHADER);
	shadowMapShaderProgram.Build(&shadowMapVertexShader, &shadowMapFragmentShader);
}

void TogglePerspective() {
	if (isProjection) {
		//projection = cyMatrix4f::MatrixScale( 1 / cameraTranslation.GetTrans().Length());
	}
	else {
		projection = glm::perspective(FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE);
	}
	isProjection = !isProjection;
}