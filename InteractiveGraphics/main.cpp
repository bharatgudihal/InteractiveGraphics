// InteractiveGraphics.cpp : Defines the entry point for the console application.
//

#include<GL/freeglut.h>

struct {
	float r, g, b, a;
}color;

void Display() {
	glClear(GL_COLOR_BUFFER_BIT);	
	glClearColor(color.r, color.g, color.b, color.a);
	glutSwapBuffers();
}

void Idle() {
	color.r -= 0.01f;
	if (color.r < 0) {
		color.r = 1.0f;
	}
	color.g -= 0.03f;
	if (color.g < 0) {
		color.g = 1.0f;
	}
	color.b -= 0.02f;
	if (color.b < 0) {
		color.b = 1.0f;
	}
	glutPostRedisplay();
}

void GetInput(unsigned char key, int xmouse, int ymouse) {
	if (key == 27) {
		glutLeaveMainLoop();
	}
}

int main(int argcp, char *argv)
{
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
	
	color = { 1.0f,1.0f,1.0f,1.0f };

	glutInit(&argcp, &argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("Test");
	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
	glutKeyboardFunc(GetInput);
	glutMainLoop();
}

