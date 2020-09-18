// -- Written in C -- //

#include<stdio.h>
#include<stdlib.h>
#include<X11/X.h>
#include<X11/Xlib.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

Display                 *dpy;
Window                  root;
GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo             *vi;
Colormap                cmap;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              glc;
XWindowAttributes       gwa;
XEvent                  xev;

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	int fd=-1;
        int bytes_read=0;

	// Read the Vertex Shader code from the file
	char VertexShaderCode[20480]={0};
	fd = open(vertex_file_path,O_RDONLY);
	if(fd < 0)
		exit(1);
        bytes_read = read(fd,VertexShaderCode,20480-1);
	if(bytes_read < 0)
		exit(2);
        close(fd);
	fd = -1;
	bytes_read=0;

	// Read the Fragment Shader code from the file
	char  FragmentShaderCode[20480]={0};
	fd = open(fragment_file_path,O_RDONLY);
	if(fd < 0)
		exit(1);
        bytes_read = read(fd,FragmentShaderCode,20480-1);
	if(bytes_read < 0)
		exit(2);
        close(fd);
	fd = -1;

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode;
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		//std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		//glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		//printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode;
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		//std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		//glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		//printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		//std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		//glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		//printf("%s\n", &ProgramErrorMessage[0]);
	}

	
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

int flag=0;
void DrawAQuad(GLuint programID,GLuint vertexbuffer) {
	if( flag == 0){
  glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
  flag =1;
  printf("1\n");
	}else{
  glClearColor(1.0, 1.0, 1.0, 1.0);
  flag =0;
  printf("0\n");
	}
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(programID);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the triangle !
  glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle

  glDisableVertexAttribArray(0);

} 
 
int main(int argc, char *argv[]) {

 dpy = XOpenDisplay(NULL);
 
 if(dpy == NULL) {
 	printf("\n\tcannot connect to X server\n\n");
        exit(0);
 }
        
 root = DefaultRootWindow(dpy);

 vi = glXChooseVisual(dpy, 0, att);

 if(vi == NULL) {
	printf("\n\tno appropriate visual found\n\n");
        exit(0);
 } 
 else {
	printf("\n\tvisual %p selected\n", (void *)vi->visualid); /* %p creates hexadecimal output like in glxinfo */
 }


 cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

 swa.colormap = cmap;
 swa.event_mask = ExposureMask | KeyPressMask;
 
 win = XCreateWindow(dpy, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

 XMapWindow(dpy, win);
 XStoreName(dpy, win, "VERY SIMPLE APPLICATION");
 
 glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
 glXMakeCurrent(dpy, win, glc);
 
 	glEnable(GL_DEPTH_TEST); 
 	GLuint VertexArrayID;
 	glGenVertexArrays(1, &VertexArrayID);
 	glBindVertexArray(VertexArrayID);
 	GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );
	static const GLfloat g_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 0.0f,  1.0f, 0.0f,
	};

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);


 
 while(1) {
 	//XNextEvent(dpy, &xev);
        //printf("event\n");
        
        //if(xev.type == Expose) {
        	XGetWindowAttributes(dpy, win, &gwa);
                glViewport(0, 0, gwa.width, gwa.height);
        	DrawAQuad(programID,vertexbuffer); 
                glXSwapBuffers(dpy, win);
        //}
                
	/*else if(xev.type == KeyPress) {
        	glXMakeCurrent(dpy, None, NULL);
 		glXDestroyContext(dpy, glc);
 		XDestroyWindow(dpy, win);
 		XCloseDisplay(dpy);
 		exit(0);
        }*/
    } /* this closes while(1) { */
} /* this is the } which closes int main(int argc, char *argv[]) { */
