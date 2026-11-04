#include "Common.h"
using namespace std;

/////////////씬 임포트/////////////
#include "Default_Scene.h"
///////////////////////////////////

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void InputProcess(GLFWwindow* window);
void DrawScene(GLFWwindow* window);
Object* Instantiate(AppContext& ctx, string name);
Object* FindObject(AppContext& ctx, string name);

map<string, GLuint> vertexShaders;
map<string, GLuint> fragmentShaders;
map<string, GLuint> shaders;

//스크린 사이즈 설정
ScreenSize screenSize{ 1920, 1080 };

//Mesh, Material 객체 캐시
map<string, Mesh*> meshChache;
map<string, Material*> materialChache;

int main(int argc, char** argv) {
	//GLFW 초기화
	if (!glfwInit()) {
		cerr << "GLFW 초기화 실패" << endl;
		return -1;
	}

	//OpenGL 버전 설정
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//윈도우 생성
	GLFWwindow* window = glfwCreateWindow(screenSize.width, screenSize.height, "OpenGL Window", nullptr, nullptr);
	if (!window) {
		cerr << "윈도우 생성 실패" << endl;
		glfwTerminate();
		return -1;
	}

	//컨텍스트 설정
	glfwMakeContextCurrent(window);

	//GLEW 초기화
	glewExperimental = GL_TRUE;	//최신 기능 사용
	if (glewInit() != GLEW_OK) {
		cerr << "GLEW 초기화 실패" << endl;
		glfwTerminate();
		return -1;
	}

	//은면 제거 활성화(Z-Buffer)
	glEnable(GL_DEPTH_TEST);

	//Back Face Culling 활성화
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CW);

	//////////////////////셰이더 초기화/////////////////////

	make_vertexShaders(vertexShaders, "Debug_vertex.glsl");
	make_fragmentShaders(fragmentShaders, "Debug_fragment.glsl");
	make_shaderProgram("Debug", shaders, vertexShaders["Debug_vertex.glsl"], fragmentShaders["Debug_fragment.glsl"]);

	make_vertexShaders(vertexShaders, "Spline_vertex.glsl");
	make_fragmentShaders(fragmentShaders, "Spline_fragment.glsl");
	make_shaderProgram("Spline", shaders, vertexShaders["Spline_vertex.glsl"], fragmentShaders["Spline_fragment.glsl"]);

	make_vertexShaders(vertexShaders, "Standard_vertex.glsl");
	make_fragmentShaders(fragmentShaders, "Standard_fragment.glsl");
	make_shaderProgram("Standard", shaders, vertexShaders["Standard_vertex.glsl"], fragmentShaders["Standard_fragment.glsl"]);

	make_vertexShaders(vertexShaders, "LocalPosColor_vertex.glsl");
	make_fragmentShaders(fragmentShaders, "LocalPosColor_fragment.glsl");
	make_shaderProgram("LocalPosColor", shaders, vertexShaders["LocalPosColor_vertex.glsl"], fragmentShaders["LocalPosColor_fragment.glsl"]);

	//////////////////Mesh, Material 캐시 초기화//////////////////

	meshChache["Cube"] = new Mesh("Default_Cube.obj");
	meshChache["Pyramid"] = new Mesh("Default_Pyramid.obj");
	meshChache["Sphere"] = new Mesh("Default_Sphere.obj");
	meshChache["Torus"] = new Mesh("Default_Torus.obj");

	Material* darkGreen = new Material(shaders["Standard"]);
	darkGreen->SetVec4("tColor", vec4(0.29f, 0.31f, 0.29f, 1.0f));
	Material* darkGray = new Material(shaders["Standard"]);
	darkGray->SetVec4("tColor", vec4(0.1f, 0.1f, 0.1f, 1.0f));
	Material* lightGray = new Material(shaders["Standard"]);
	lightGray->SetVec4("tColor", vec4(0.34f, 0.34f, 0.39f, 1.0f));
	Material* darkYellow = new Material(shaders["Standard"]);
	darkYellow->SetVec4("tColor", vec4(0.72f, 0.66f, 0.24f, 1.0f));
	Material* black = new Material(shaders["Standard"]);
	black->SetVec4("tColor", vec4(0.0f, 0.0f, 0.0f, 1.0f));
	Material* red = new Material(shaders["Standard"]);
	red->SetVec4("tColor", vec4(1.0f, 0.0f, 0.0f, 1.0f));
	Material* green = new Material(shaders["Standard"]);
	green->SetVec4("tColor", vec4(0.0f, 1.0f, 0.0f, 1.0f));
	Material* blue = new Material(shaders["Standard"]);
	blue->SetVec4("tColor", vec4(0.0f, 0.0f, 1.0f, 1.0f));
	Material* orange = new Material(shaders["Standard"]);
	orange->SetVec4("tColor", vec4(1.0f, 0.5f, 0.0f, 1.0f));
	Material* purple = new Material(shaders["Standard"]);
	purple->SetVec4("tColor", vec4(0.5f, 0.0f, 1.0f, 1.0f));

	materialChache["LocalPosColor"] = new Material(shaders["LocalPosColor"]);
	materialChache["DarkGreen"] = darkGreen;
	materialChache["DarkGray"] = darkGray;
	materialChache["LightGray"] = lightGray;
	materialChache["DarkYellow"] = darkYellow;
	materialChache["Black"] = black;
	materialChache["Red"] = red;
	materialChache["Green"] = green;
	materialChache["Blue"] = blue;
	materialChache["Orange"] = orange;
	materialChache["Purple"] = purple;

	//////////////////사용자 정의 초기화////////////////////

	//외부 입력 콜백 함수 설정
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);

	//윈도우 크기 변경 시 screenSize를 동기화해 주는 콜백
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		screenSize.width = width;
		screenSize.height = height;
		});

	//컨텍스트 변수 생성, 윈도우 객체에 연결
	AppContext ctx;
	glfwSetWindowUserPointer(window, &ctx);
	ctx.bgColor = { 0.2f, 0.2f, 0.2f, 1.0f }; //배경색 설정

	ColliderManager colliderManager(ctx);

	//시간 초기화
	ctx.time = glfwGetTime();

	/////////////////////////씬 로드/////////////////////////

	Default_Scene defaultScene{screenSize};
	defaultScene.LoadScene(ctx);

	////////////////////////메인 루프////////////////////////

	while (!glfwWindowShouldClose(window)) {
		//시간 계산
		double currentTime = glfwGetTime();
		ctx.deltaTime = currentTime - ctx.time;
		ctx.deltaTime = std::min(ctx.deltaTime, 0.05); //델타타임 제한(너무 큰 값 방지)
		ctx.time = currentTime;

		//입력 처리
		ctx.inputManager.Update();
		InputProcess(window);

		//이벤트 처리
		glfwPollEvents();

		//Start 처리
		for (auto& obj : ctx.Hierarchy) {
			if (obj->isValid && !obj->isStarted) {
				obj->isStarted = true;
				obj->Start();
			}
		}

		//Update 처리
		for (auto& obj : ctx.Hierarchy) {
			if (obj->isValid) obj->Update(ctx.deltaTime);
		}

		//충돌 감지
		colliderManager.Update();

		//추가할 오브젝트가 있다면 Hierarchy에 추가
		if (!ctx.pendingHierarchy.empty()) {
			for (auto& newObj : ctx.pendingHierarchy) {
				ctx.Hierarchy.push_back(move(newObj));
			}
			ctx.pendingHierarchy.clear();
		}

		//추가할 충돌 감지 콜리전이 있다면 CollisionObjects에 추가
		if (!ctx.pendingCollisionObjects.empty()) {
			for (auto& newCol : ctx.pendingCollisionObjects) {
				ctx.CollisionObjects.push_back(newCol);
			}
			ctx.pendingCollisionObjects.clear();
		}

		//추가할 카메라가 있다면 cameras에 추가
		if (!ctx.pendingCameras.empty()) {
			for (auto& newCam : ctx.pendingCameras) {
				ctx.cameras.push_back(newCam);
			}
			ctx.pendingCameras.clear();
		}

		//제거할 오브젝트가 있다면 Hierarchy에서 제거
		if (!ctx.pendingDestroyObjects.empty()) {
			for (auto& targetObj : ctx.pendingDestroyObjects) {
				//CollisionObjects에서도 제거
				auto& collist = ctx.CollisionObjects;
				collist.erase(remove_if(collist.begin(), collist.end(),
					[targetObj](Collider* col) {
						return col->gameObject == targetObj;
					}),
					collist.end());

				//cameras에서도 제거
				auto& camlist = ctx.cameras;
				camlist.erase(remove_if(camlist.begin(), camlist.end(),
					[targetObj](Camera* cam) {
						return cam->gameObject == targetObj;
					}),
					camlist.end());

				//부모 자식 관계 정리
				Transform* targetTr = targetObj->GetComponent<Transform>();
				if (targetTr && targetTr->parent != nullptr) {
					auto& siblings = targetTr->parent->children;
					siblings.erase(remove(siblings.begin(), siblings.end(), targetTr), siblings.end());
				}

				//Hierarchy에서 제거
				auto& hierarchy = ctx.Hierarchy;
				hierarchy.erase(remove_if(hierarchy.begin(), hierarchy.end(),
					[targetObj](const unique_ptr<Object>& obj) { return obj.get() == targetObj; }),
					hierarchy.end());
			}
			ctx.pendingDestroyObjects.clear();
		}

		//화면 렌더링
		DrawScene(window);
	}

	/////////////////////////루프 종료/////////////////////////
	//리소스 해제
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
	if (!ctx) return;
	
	if (action == GLFW_PRESS) ctx->inputManager.SetKey(key, true);
	else if (action == GLFW_RELEASE) ctx->inputManager.SetKey(key, false);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
	if (!ctx) return;

	if (action == GLFW_PRESS) ctx->inputManager.SetKey(button, true);
	else if (action == GLFW_RELEASE) ctx->inputManager.SetKey(button, false);
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
	if (!ctx) return;

	//마우스 좌표를 OpenGL 좌표계로 변환
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	float glX = (xpos / width) * 2.0f - 1.0f;
	float glY = 1.0f - (ypos / height) * 2.0f; //Y축 반전
	ctx->inputManager.SetMousePosition(glX, glY);
}

void InputProcess(GLFWwindow* window)
{
	AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	/*else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}*/
}

void DrawScene(GLFWwindow* window)
{
	AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

	//버퍼 초기화
	/*glClearColor(ctx->bgColor.r, ctx->bgColor.g, ctx->bgColor.b, ctx->bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);*/

	//활성화 되어 있는 카메라 추출
	vector<Camera*> activeCameras;
	for (Camera* cam : ctx->cameras) {
		if (cam && cam->gameObject && cam->gameObject->isValid) activeCameras.push_back(cam);
	}

	//카메라가 없을때 배경색으로 화면 초기화 후 버퍼 스왑
	if (activeCameras.empty()) {
		glClearColor(ctx->bgColor.r, ctx->bgColor.g, ctx->bgColor.b, ctx->bgColor.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwSwapBuffers(window);
		return;
	}

	//카메라 우선순위에 따라 정렬
	stable_sort(activeCameras.begin(), activeCameras.end(), [](Camera* a, Camera* b) {
		return a->priority < b->priority;
	});

	//렌더링 큐 생성
	vector<Object*> renderQueue;
	renderQueue.reserve(ctx->Hierarchy.size());
	for (auto& obj : ctx->Hierarchy) {
		renderQueue.push_back(obj.get());
	}

	//렌더링 큐 정렬 (Z값 기준. Z값이 클 수록 나중에 렌더링)
	stable_sort(renderQueue.begin(), renderQueue.end(),
		[](Object* a, Object* b) {
			Transform* trA = a->GetComponent<Transform>();
			Transform* trB = b->GetComponent<Transform>();
			if (trA && trB) {
				return trA->worldPosition.z < trB->worldPosition.z;
			}
			return false;
		});

	//각 카메라 시점별 렌더링
	for (Camera* cam : activeCameras) {
		int vpX = static_cast<GLint>(cam->viewportRect.x * screenSize.width);
		int vpY = static_cast<GLint>(cam->viewportRect.y * screenSize.height);
		int vpW = static_cast<GLsizei>(cam->viewportRect.z * screenSize.width);
		int vpH = static_cast<GLsizei>(cam->viewportRect.w * screenSize.height);

		if (vpW <= 0 || vpH <= 0) continue;

		cam->aspectRatio = static_cast<float>(vpW) / static_cast<float>(vpH);

		glViewport(vpX, vpY, vpW, vpH);

		glEnable(GL_SCISSOR_TEST);
		glScissor(vpX, vpY, vpW, vpH);

		GLbitfield clearFlags = 0;
		if (cam->clearColorBuffer) {
			glClearColor(cam->backgroundColor.r, cam->backgroundColor.g, cam->backgroundColor.b, cam->backgroundColor.a);
			clearFlags |= GL_COLOR_BUFFER_BIT;
		}
		if (cam->clearDepthBuffer) {
			clearFlags |= GL_DEPTH_BUFFER_BIT;
		}
		if (clearFlags != 0) glClear(clearFlags);

		glDisable(GL_SCISSOR_TEST);

		// 현재 그리려는 카메라 객체를 전달하여 렌더링
		for (auto& obj : renderQueue) {
			obj->Render(cam);
		}
	}

	//버퍼 스왑
	glfwSwapBuffers(window);
}