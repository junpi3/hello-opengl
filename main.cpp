#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <miniaudio.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

// Utility to load shader source from file
std::string loadShaderSource(const char* filePath) {
  std::ifstream file(filePath);
  if (!file) {
    std::cerr << "Failed to open shader file: " << filePath << "\n";
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

constexpr int kWindowW = 1200;
constexpr int kWindowH = 600;

// Fullscreen quad for world map (constexpr)
constexpr float kMapVerts[] = {
  // positions   // tex coords
  -1.0f,  1.0f,  0.0f, 1.0f, // top-left
  -1.0f, -1.0f,  0.0f, 0.0f, // bottom-left
   1.0f, -1.0f,  1.0f, 0.0f, // bottom-right
   1.0f,  1.0f,  1.0f, 1.0f  // top-right
};

// Kopi quad half-width and half-height
constexpr float kKopiHalfW = 0.1f;
constexpr float kKopiHalfH = 0.24f;

// Quad for kopi (smaller, centered at offset, uses kKopiHalfW/kKopiHalfH)
constexpr float kKopiVerts[] = {
  // positions         // tex coords
  -kKopiHalfW,  kKopiHalfH,  0.0f, 1.0f, // top-left
  -kKopiHalfW, -kKopiHalfH,  0.0f, 0.0f, // bottom-left
   kKopiHalfW, -kKopiHalfH,  1.0f, 0.0f, // bottom-right
   kKopiHalfW,  kKopiHalfH,  1.0f, 1.0f  // top-right
};

constexpr unsigned int kIdxs[] = {
  0, 1, 2,
  0, 2, 3
};

enum Quadrant: uint8_t { TOP_RIGHT = 0, TOP_LEFT = 1, BOTTOM_LEFT = 2, BOTTOM_RIGHT = 3 };

struct KopiState {
  bool isPressed = false;
  double lastX = 0.0f, lastY = 0.0f;
  float offX = 0.0f, offY = 0.0f;
  float angle = 0.0f; // in radians
  Quadrant lastQ = TOP_RIGHT;

  Quadrant curQ() const {
    if (offX >= 0 && offY >= 0) return TOP_RIGHT;
    if (offX < 0 && offY >= 0)  return TOP_LEFT;
    if (offX < 0 && offY < 0)   return BOTTOM_LEFT;
    return BOTTOM_RIGHT;
  }
};

constexpr char* kWavFiles[] = {
  "res/first.wav",
  "res/second.wav",
  "res/third.wav",
  "res/fourth.wav"
};

static ma_sound kSounds[4];

void updateSound(KopiState* k) {
  Quadrant curQ = k->curQ();
  Quadrant lastQ = k->lastQ;
  if (curQ != lastQ) {
    ma_sound_stop(&kSounds[lastQ]);
    // Start sound from the beginning
    ma_sound_seek_to_pcm_frame(&kSounds[curQ], 0);
    ma_sound_start(&kSounds[curQ]);
    k->lastQ = curQ;
  }
}

// Helper to check if mouse is inside kopi quad (NDC coordinates)
bool isMouseInKopi(GLFWwindow* window, double xpos, double ypos, float xoff, float yoff, float angle = 0.0f) {
  int winW, winH;
  glfwGetWindowSize(window, &winW, &winH);
  // Convert window coordinates to NDC
  float xNdc = (xpos / winW) * 2.0f - 1.0f;
  float yNdc = 1.0f - (ypos / winH) * 2.0f;

  // Undo rotation for hit test
  float dx = xNdc - xoff;
  float dy = yNdc - yoff;
  float cosA = cos(-angle);
  float sinA = sin(-angle);
  float xr = dx * cosA - dy * sinA;
  float yr = dx * sinA + dy * cosA;

  return xr >= -kKopiHalfW && xr <= kKopiHalfW &&
         yr >= -kKopiHalfH && yr <= kKopiHalfH;
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  KopiState* k = static_cast<KopiState*>(glfwGetWindowUserPointer(window));
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);
      if (isMouseInKopi(window, xpos, ypos, k->offX, k->offY, k->angle)) {
        k->isPressed = true;
        k->lastX = xpos;
        k->lastY = ypos;
      }
    } else if (action == GLFW_RELEASE) {
      k->isPressed = false;
      updateSound(k);
    }
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (isMouseInKopi(window, xpos, ypos, k->offX, k->offY, k->angle)) {
      // Rotate 45 degrees clockwise
      k->angle += 3.14159265f / 4.0f;
      if (k->angle > 3.14159265f * 2.0f)
        k->angle -= 3.14159265f * 2.0f;
    }
  }
}

// Mouse move callback
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  KopiState* k = static_cast<KopiState*>(glfwGetWindowUserPointer(window));
  if (k->isPressed) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float dx = (xpos - k->lastX) / (width / 2.0f);
    float dy = (k->lastY - ypos) / (height / 2.0f); // invert y
    k->offX += dx;
    k->offY += dy;
    k->lastX = xpos;
    k->lastY = ypos;
  }
}

GLuint loadTexture(const char* path, int* outWidth = nullptr, int* outHeight = nullptr) {
  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
  if (!data) {
    std::cerr << "Failed to load texture: " << path << "\n";
    return 0;
  }
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data);
  if (outWidth) *outWidth = width;
  if (outHeight) *outHeight = height;
  return texture;
}

int main() {
  // Initialize miniaudio engine
  ma_engine engine;
  if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
    std::cerr << "Failed to initialize miniaudio engine\n";
    return -1;
  }

  // Initialize wav files (static sounds array)
  for (int i = 0; i < 4; ++i) {
    if (ma_sound_init_from_file(&engine, kWavFiles[i], 0, NULL, NULL, &kSounds[i]) != MA_SUCCESS) {
      std::cerr << "Failed to load " << kWavFiles[i] << "\n";
      ma_engine_uninit(&engine);
      return -1;
    }
    ma_sound_set_looping(&kSounds[i], MA_TRUE);
  }
  // Start the first sound by default
  ma_sound_start(&kSounds[0]);

  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create window
  GLFWwindow* window = glfwCreateWindow(kWindowW, kWindowH, "World Map", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Load OpenGL functions
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
    return -1;
  }

  KopiState kopiState;
  glfwSetWindowUserPointer(window, &kopiState);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);

  // Load shaders from files
  std::string vtxShaderMap = loadShaderSource("glsl/vertex_map.glsl");
  std::string vtxShaderKopi = loadShaderSource("glsl/vertex_kopi.glsl");
  std::string fragShader = loadShaderSource("glsl/fragment.glsl");
  const char* mShaderSrc = vtxShaderMap.c_str();
  const char* kShaderSrc = vtxShaderKopi.c_str();
  const char* fShaderSrc = fragShader.c_str();

  GLuint mVtxShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(mVtxShader, 1, &mShaderSrc, nullptr);
  glCompileShader(mVtxShader);
  
  GLuint kVtxShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(kVtxShader, 1, &kShaderSrc, nullptr);
  glCompileShader(kVtxShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fShaderSrc, nullptr);
  glCompileShader(fragmentShader);

  GLuint mapShaderProgram = glCreateProgram();
  glAttachShader(mapShaderProgram, mVtxShader);
  glAttachShader(mapShaderProgram, fragmentShader);
  glLinkProgram(mapShaderProgram);

  GLuint kopiShaderProgram = glCreateProgram();
  glAttachShader(kopiShaderProgram, kVtxShader);
  glAttachShader(kopiShaderProgram, fragmentShader);
  glLinkProgram(kopiShaderProgram);

  glDeleteShader(mVtxShader);
  glDeleteShader(kVtxShader);
  glDeleteShader(fragmentShader);

  GLuint mapVBO, mapVAO, mapEBO;
  glGenVertexArrays(1, &mapVAO);
  glGenBuffers(1, &mapVBO);
  glGenBuffers(1, &mapEBO);

  glBindVertexArray(mapVAO);
  glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kMapVerts), kMapVerts, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mapEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIdxs), kIdxs, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  GLuint kopiVBO, kopiVAO, kopiEBO;
  glGenVertexArrays(1, &kopiVAO);
  glGenBuffers(1, &kopiVBO);
  glGenBuffers(1, &kopiEBO);

  glBindVertexArray(kopiVAO);
  glBindBuffer(GL_ARRAY_BUFFER, kopiVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kKopiVerts), kKopiVerts, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, kopiEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIdxs), kIdxs, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Load textures
  GLuint mapTexture = loadTexture("res/world_map.png");
  GLuint kopiTexture = loadTexture("res/kopi.png");
  if (!mapTexture || !kopiTexture) {
    return -1;
  }

  // Render loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw world map
    glUseProgram(mapShaderProgram);
    glBindVertexArray(mapVAO);
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Draw kopi overlay
    glUseProgram(kopiShaderProgram);
    GLint offsetLoc = glGetUniformLocation(kopiShaderProgram, "offset");
    GLint angleLoc = glGetUniformLocation(kopiShaderProgram, "angle");
    GLint aspectLoc = glGetUniformLocation(kopiShaderProgram, "aspect");

    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    float aspect = static_cast<float>(winH) / winW;

    glBindVertexArray(kopiVAO);
    glBindTexture(GL_TEXTURE_2D, kopiTexture);
    glUniform2f(offsetLoc, kopiState.offX, kopiState.offY);
    glUniform1f(angleLoc, kopiState.angle);
    glUniform1f(aspectLoc, aspect);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &mapVAO);
  glDeleteBuffers(1, &mapVBO);
  glDeleteBuffers(1, &mapEBO);
  glDeleteVertexArrays(1, &kopiVAO);
  glDeleteBuffers(1, &kopiVBO);
  glDeleteBuffers(1, &kopiEBO);
  glDeleteProgram(mapShaderProgram);
  glDeleteProgram(kopiShaderProgram);
  glDeleteTextures(1, &mapTexture);
  glDeleteTextures(1, &kopiTexture);
  for (int i = 0; i < 4; ++i) ma_sound_uninit(&kSounds[i]);
  ma_engine_uninit(&engine);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}