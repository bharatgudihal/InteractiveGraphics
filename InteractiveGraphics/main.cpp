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
const float FOV = 0.785398f;
const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 1000.0f;
const float CUBE_MAP_MESH_SIZE = 500.0f;

//Teapot model variables
cyTriMesh teapotMesh;
GLuint teapotVAO, teapotVBO, teapotDiffuseTextureId, teapotSpecularTextureId;
cyGLSLShader teapotVertexShader, teapotFragmentShader;
cyGLSLProgram teapotShaderProgram;
Material teapotMaterial;
cyMatrix4f teapotModelMatrix = cyMatrix4f::MatrixTrans(cyPoint3f(0, 0, 0));

//Camera variables
cyMatrix4f cameraRotation = cyMatrix4f::MatrixRotationX(0);
cyMatrix4f cameraTranslation = cyMatrix4f::MatrixTrans(cyPoint3f(0,0,-70.0f));
bool isProjection = true;
float xCameraRot = 0.0f;
float yCameraRot = 0.0f;
cyMatrix4f projection = cyMatrix4f::MatrixPerspective(FOV, WINDOW_WIDTH / (float)WINDOW_HEIGHT, NEAR_PLANE, FAR_PLANE);

//Light variables
cyPoint3f lightPosition(70.0f, 70.0f, 70.0f);
bool isLightRotating;
float xLightRot = 0.0f;
float yLightRot = 0.0f;

//Plane variables
GLuint planeVAO, planeVBO, planeDiffuseTextureId;
cyGLSLShader planeVertexShader, planeFragmentShader;
cyGLSLProgram planeShaderProgram;
cyMatrix4f planeModelMatrix;
cyMatrix4f planeTranslation = cyMatrix4f::MatrixTrans(cyPoint3f(0, 0, -10.0f));
bool isPlaneMoving;
float xPlaneRot = 0.0f;
float yPlaneRot = 0.0f;

//Render texture variables
cyGLRenderTexture2D renderTexture;

//General variables
cyMatrix4f mvp;
float lastRightMousePos;
float lastLeftMousePosX;
float lastLeftMousePosY;
float rotationSpeed = 0.0174533f;
float xRot = 0.0f;
float yRot = 0.0f;
int pressedButton;

//Cube map variables
static std::string cubeTexturePath = "Assets/Models/cube/cube.obj";
cyTriMesh cubeMesh;
GLuint cubeVAO, cubeVBO;
cyGLTextureCubeMap cubeMapTexture;
cyGLSLShader cubeMapVertexShader, cubeMapFragmentShader;
cyGLSLProgram cubeMapShaderProgram;

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
			teapotModelMatrix = cyMatrix4f::MatrixRotationX(-1.5708f) * cyMatrix4f::MatrixTrans(-(teapotMesh.GetBoundMax() + teapotMesh.GetBoundMin()) / 2.0f);
			if (!teapotMesh.HasNormals()) {
				teapotMesh.ComputeNormals();
			}

			//Extract all vertex data
			VertexData* vertices = new VertexData[teapotMesh.NF() * 3];
			for (unsigned int i = 0; i < teapotMesh.NF(); i++) {
				cyTriMesh::TriFace face = teapotMesh.F(i);
				cyTriMesh::TriFace normalFace = teapotMesh.FN(i);
				cyTriMesh::TriFace textureFace = teapotMesh.FT(i);
				unsigned int vertexIndex = i * 3;
				vertices[vertexIndex].vertexPosition = teapotMesh.V(face.v[0]);
				vertices[vertexIndex + 1].vertexPosition = teapotMesh.V(face.v[1]);
				vertices[vertexIndex + 2].vertexPosition = teapotMesh.V(face.v[2]);

				vertices[vertexIndex].vertexNormal = teapotMesh.VN(normalFace.v[0]);
				vertices[vertexIndex + 1].vertexNormal = teapotMesh.VN(normalFace.v[1]);
				vertices[vertexIndex + 2].vertexNormal = teapotMesh.VN(normalFace.v[2]);

				vertices[vertexIndex].uv = cyPoint2f(teapotMesh.VT(textureFace.v[0]));
				vertices[vertexIndex + 1].uv = cyPoint2f(teapotMesh.VT(textureFace.v[1]));
				vertices[vertexIndex + 2].uv = cyPoint2f(teapotMesh.VT(textureFace.v[2]));
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
			
			CompileShaders();
			
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
			teapotMaterial.diffuse = cyPoint3f(0.92f, 0.92f, 0.92f);
			teapotMaterial.ambient = teapotMaterial.diffuse;
			teapotMaterial.specular = cyPoint3f(1.0f, 1.0f, 1.0f);
			teapotMaterial.shininess = 50.0f;
		}

		//Setup plane
		{
			const float planeHalfWidth = 3.0f;
			const float planeHalfHeight = 3.0f;
			VertexData* vertices = new VertexData[6];
			vertices[0].vertexPosition = cyPoint3f(-planeHalfWidth, -planeHalfHeight, 0.0f);
			vertices[0].uv = cyPoint2f(0.0f, 0.0f);
			vertices[1].vertexPosition = cyPoint3f(planeHalfWidth, -planeHalfHeight, 0.0f);
			vertices[1].uv = cyPoint2f(1.0f, 0.0f);
			vertices[2].vertexPosition = cyPoint3f(-planeHalfWidth, planeHalfHeight, 0.0f);
			vertices[2].uv = cyPoint2f(0.0f, 1.0f);
			vertices[3].vertexPosition = cyPoint3f(planeHalfWidth, -planeHalfHeight, 0.0f);
			vertices[3].uv = cyPoint2f(1.0f, 0.0f);
			vertices[4].vertexPosition = cyPoint3f(planeHalfWidth, planeHalfHeight, 0.0f);
			vertices[4].uv = cyPoint2f(1.0f, 1.0f);
			vertices[5].vertexPosition = cyPoint3f(-planeHalfWidth, planeHalfHeight, 0.0f);
			vertices[5].uv = cyPoint2f(0.0f, 1.0f);

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

			planeVertexShader.CompileFile("Assets/Shaders/Vertex/plane.glsl", GL_VERTEX_SHADER);
			planeFragmentShader.CompileFile("Assets/Shaders/Fragment/plane.glsl", GL_FRAGMENT_SHADER);
			planeShaderProgram.Build(&planeVertexShader, &planeFragmentShader);

			delete[] vertices;
		}

		//Setup render texture
		{
			const int numChannels = 3;
			renderTexture.Initialize(true, numChannels, WINDOW_WIDTH, WINDOW_HEIGHT);
			//Bilinear filtering for magnification
			renderTexture.SetTextureFilteringMode(GL_LINEAR, 0);
			//Anisotropic filtering for minification
			renderTexture.SetTextureMaxAnisotropy();
			renderTexture.BuildTextureMipmaps();
		}

		//Setup cube map
		if (cubeMesh.LoadFromFileObj(cubeTexturePath.c_str())) {
			
			//Extract all vertex data
			VertexData* vertices = new VertexData[cubeMesh.NF() * 3];
			for (unsigned int i = 0; i < cubeMesh.NF(); i++) {
				
				cyTriMesh::TriFace face = cubeMesh.F(i);
				
				unsigned int vertexIndex = i * 3;
				
				//Multiply vertex position with the size
				vertices[vertexIndex].vertexPosition = cubeMesh.V(face.v[0]) * CUBE_MAP_MESH_SIZE;
				vertices[vertexIndex + 1].vertexPosition = cubeMesh.V(face.v[1]) * CUBE_MAP_MESH_SIZE;
				vertices[vertexIndex + 2].vertexPosition = cubeMesh.V(face.v[2]) * CUBE_MAP_MESH_SIZE;
			}

			glGenVertexArrays(1, &cubeVAO);
			glGenBuffers(1, &cubeVBO);
			glBindVertexArray(cubeVAO);
			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
			glBufferData(GL_ARRAY_BUFFER, cubeMesh.NF() * 3 * sizeof(VertexData), vertices, GL_STATIC_DRAW);
			
			// Position vertex attribute
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexPosition));			

			delete[] vertices;

			//Initialize shaders
			{
				cubeMapVertexShader.CompileFile("Assets/Shaders/Vertex/cubemap.glsl", GL_VERTEX_SHADER);
				cubeMapFragmentShader.CompileFile("Assets/Shaders/Fragment/cubemap.glsl", GL_FRAGMENT_SHADER);
				cubeMapShaderProgram.Build(&cubeMapVertexShader, &cubeMapFragmentShader);
			}

			//Initialize cube map
			{	
				//Load and assign cube map textures
				std::vector<unsigned char> imageBuffer;
				unsigned width, height;
				bool result;

				//NegX
				result = lodepng::decode(imageBuffer, width, height, "Assets/Cubemaps/cubemap_negx.png", LodePNGColorType::LCT_RGB);
				assert(result == 0);
				cubeMapTexture.SetImage(cy::GLTextureCubeMap::NEGATIVE_X, imageBuffer.data(), 3, width, height);
				imageBuffer.clear();
				
				//NegY
				result = lodepng::decode(imageBuffer, width, height, "Assets/Cubemaps/cubemap_negy.png", LodePNGColorType::LCT_RGB);
				assert(result == 0);
				cubeMapTexture.SetImage(cy::GLTextureCubeMap::NEGATIVE_Y, imageBuffer.data(), 3, width, height);
				imageBuffer.clear();

				//NegZ
				result = lodepng::decode(imageBuffer, width, height, "Assets/Cubemaps/cubemap_negz.png", LodePNGColorType::LCT_RGB);
				assert(result == 0);
				cubeMapTexture.SetImage(cy::GLTextureCubeMap::NEGATIVE_Z, imageBuffer.data(), 3, width, height);
				imageBuffer.clear();

				//PosX
				result = lodepng::decode(imageBuffer, width, height, "Assets/Cubemaps/cubemap_posx.png", LodePNGColorType::LCT_RGB);
				assert(result == 0);
				cubeMapTexture.SetImage(cy::GLTextureCubeMap::POSITIVE_X, imageBuffer.data(), 3, width, height);
				imageBuffer.clear();

				//PosY
				result = lodepng::decode(imageBuffer, width, height, "Assets/Cubemaps/cubemap_posy.png", LodePNGColorType::LCT_RGB);
				assert(result == 0);
				cubeMapTexture.SetImage(cy::GLTextureCubeMap::POSITIVE_Y, imageBuffer.data(), 3, width, height);
				imageBuffer.clear();

				//PosZ
				result = lodepng::decode(imageBuffer, width, height, "Assets/Cubemaps/cubemap_posz.png", LodePNGColorType::LCT_RGB);
				assert(result == 0);
				cubeMapTexture.SetImage(cy::GLTextureCubeMap::POSITIVE_Z, imageBuffer.data(), 3, width, height);
				imageBuffer.clear();


				cubeMapTexture.SetFilteringMode(GL_LINEAR, GL_LINEAR);
				cubeMapTexture.SetSeamless(true);
				cubeMapTexture.BuildMipmaps();
				
			}
		}

		glutMainLoop();		

		//Cleanup
		renderTexture.Delete();
	}
	else {
		std::cerr << "File not found" << std::endl;
	}
}

void Display() {

	//Set clear color
	glClearColor(color.r, color.g, color.b, color.a);	

	//Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//Setup matrices for teapot
	const cyMatrix4f view = cameraTranslation * cameraRotation;
	mvp = projection * view * teapotModelMatrix;
	cyMatrix3f cameraMatrix = (view * teapotModelMatrix).GetSubMatrix3();
	cyMatrix4f worldToModel = cyMatrix4f::MatrixRotationY(yLightRot) * cyMatrix4f::MatrixRotationX(xLightRot);
	cyMatrix4f lightModelMatrix = worldToModel * cyMatrix4f::MatrixTrans(lightPosition);
	cyPoint3f transformedLightPos = lightModelMatrix.GetTrans();
	cyPoint3f cameraPosition = view.GetTrans();
	
	//Set all shader params for teapot
	{
		teapotShaderProgram.Bind();
		teapotShaderProgram.SetUniformMatrix4("mvp", mvp.data);
		teapotShaderProgram.SetUniformMatrix4("cameraMatrix", (view * teapotModelMatrix).data);
		cameraMatrix.Invert();
		cameraMatrix.Transpose();
		teapotShaderProgram.SetUniformMatrix3("normalMatrix", cameraMatrix.data);
		teapotShaderProgram.SetUniform("lightPosition", transformedLightPos);
		teapotShaderProgram.SetUniform("ambient", teapotMaterial.ambient);
		teapotShaderProgram.SetUniform("diffuse", teapotMaterial.diffuse);
		teapotShaderProgram.SetUniform("specular", teapotMaterial.specular);
		teapotShaderProgram.SetUniform("shininess", teapotMaterial.shininess);
		teapotShaderProgram.SetUniform("cameraPosition", cameraPosition);
	}
	
	//Set active texture and bind for teapot
	if (teapotMesh.NM() > 0) {
		{
			//Diffuse
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, teapotDiffuseTextureId);

			//Specular
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, teapotSpecularTextureId);
		}
		teapotShaderProgram.SetUniform("hasDiffuse", true);
		teapotShaderProgram.SetUniform("hasSpecular", true);
	}
	else {
		teapotShaderProgram.SetUniform("hasDiffuse", false);
		teapotShaderProgram.SetUniform("hasSpecular", false);
	}

	//Draw teapot
	{
		glBindVertexArray(teapotVAO);
		glDrawArrays(GL_TRIANGLES, 0, teapotMesh.NF() * 3);
	}

	//Disable depth buffer
	glDepthMask(GL_FALSE);

	//Set shader values
	{
		//Place cube so that camera is in the center and rotate it in the opposite direction
		cyMatrix4f cubeMVP = projection * cyMatrix4f::MatrixRotationY(-yCameraRot) * cyMatrix4f::MatrixRotationX(-xCameraRot);
		cubeMapShaderProgram.SetUniformMatrix4("mvp", cubeMVP.data);
	}

	//Bind cube texture
	{
		cubeMapTexture.Bind();
	}

	//Draw cube
	{
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, cubeMesh.NF() * 3);
	}

	//Enable depth buffer
	glDepthMask(GL_TRUE);

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
			cameraRotation = cyMatrix4f::MatrixRotationY(yCameraRot) * cyMatrix4f::MatrixRotationX(xCameraRot);
		}
	}else {
		float diff = static_cast<float>(y) - lastRightMousePos;
		lastRightMousePos = static_cast<float>(y);
		if (isPlaneMoving) {
			planeTranslation.AddTrans(-cyPoint3f(0.0f, 0.0f, diff));
		}
		else {
			cameraTranslation.AddTrans(-cyPoint3f(0.0f, 0.0f, diff));
		}
	}
}

void CompileShaders() {
	teapotVertexShader.CompileFile("Assets/Shaders/Vertex/teapot.glsl", GL_VERTEX_SHADER);
	teapotFragmentShader.CompileFile("Assets/Shaders/Fragment/teapot.glsl", GL_FRAGMENT_SHADER);
	teapotShaderProgram.Build(&teapotVertexShader, &teapotFragmentShader);
}

void TogglePerspective() {
	if (isProjection) {
		projection = cyMatrix4f::MatrixScale( 1 / cameraTranslation.GetTrans().Length());
	}
	else {
		projection = cyMatrix4f::MatrixPerspective(0.785398f, WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100);
	}
	isProjection = !isProjection;
}