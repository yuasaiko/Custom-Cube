#include <cmath>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS        // ラジアン単位の角度を使うことを強制する
#define GLM_ENABLE_EXPERIMENTAL  // glm/gtx/**.hppを使うのに必要
#include <glm/glm.hpp>
// glm::vec型をポインタに変換 / Convert glm::vec types to pointer
#include <glm/gtc/type_ptr.hpp>
// GLMの行列変換のためのユーティリティ関数 GLM's utility functions for matrix transformation
#include <glm/gtx/transform.hpp>

#define STB_IMAGE_IMPLEMENTATION  // 画像のロードに必要 / Required to load images
#include "stb_image.h"

// 画像のパスなどが書かれた設定ファイル
// Config file storing image locations etc.
#include "common.h"

static int WIN_WIDTH = 500;                      // ウィンドウの幅 / Window width
static int WIN_HEIGHT = 500;                     // ウィンドウの高さ / Window height
static const char *WIN_TITLE = "OpenGL Course";  // ウィンドウのタイトル / Window title

static  bool ArtMode = true;
const std::string SETTING_IMAGE = std::string(DATA_DIRECTORY) + "setting.png"; // 設定画面の画像パス / Path to the settings image
static const std::string TEX_FILE = std::string(DATA_DIRECTORY) + "yu.png"; 
const std::string TEX_FILES[6] = {
    std::string(DATA_DIRECTORY) + "face0.png", // +X
    std::string(DATA_DIRECTORY) + "face1.png", // -X
    std::string(DATA_DIRECTORY) + "face2.png", // +Y
    std::string(DATA_DIRECTORY) + "face3.png", // -Y
    std::string(DATA_DIRECTORY) + "face4.png", // +Z
    std::string(DATA_DIRECTORY) + "face5.png"  // -Z
};
GLuint textureIds[6];

// シェーダ言語のソースファイル / Shader source files
static std::string VERT_SHADER_FILE = std::string(SHADER_DIRECTORY) + "render.vert";
static std::string FRAG_SHADER_FILE = std::string(SHADER_DIRECTORY) + "render.frag";

// 頂点クラス
// Vertex class
struct Vertex {
    Vertex(const glm::vec3 &position_, const glm::vec3 &color_, const glm::vec2 &texcoord_ = glm::vec2(0.0f))
        : position(position_)
        , color(color_)
        , texcoord(texcoord_) {
    }

    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texcoord; // 追加
};

// clang-format off
static const glm::vec3 positions[8] = {
    glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3( 1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f,  1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f,  1.0f),
    glm::vec3( 1.0f,  1.0f, -1.0f),
    glm::vec3(-1.0f,  1.0f,  1.0f),
    glm::vec3( 1.0f, -1.0f,  1.0f),
    glm::vec3( 1.0f,  1.0f,  1.0f)
};

static const glm::vec3 colors[8] = {
    glm::vec3(1.0f, 0.0f, 0.0f),  // 赤
    glm::vec3(0.0f, 1.0f, 0.0f),  // 緑
    glm::vec3(0.0f, 0.0f, 1.0f),  // 青
    glm::vec3(1.0f, 1.0f, 0.0f),  // イエロー
    glm::vec3(0.0f, 1.0f, 1.0f),  // シアン
    glm::vec3(1.0f, 0.0f, 1.0f),  // マゼンタ
    glm::vec3(0.0f, 0.0f, 0.0f),  // 黒
    glm::vec3(1.0f, 1.0f, 1.0f)   // 白
};

static const unsigned int faces[12][3] = {
    { 7, 4, 1 }, { 7, 1, 6 },
    { 2, 4, 7 }, { 2, 7, 5 },
    { 5, 7, 6 }, { 5, 6, 3 },
    { 4, 2, 0 }, { 4, 0, 1 },
    { 3, 6, 1 }, { 3, 1, 0 },
    { 2, 5, 3 }, { 2, 3, 0 }
};


GLuint textureId;
GLuint settingTexId;

// グローバル変数
int settingImgWidth = 1, settingImgHeight = 1;

void loadSettingTexture() {
    int width, height, channels;
    unsigned char *data = stbi_load(SETTING_IMAGE.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "Failed to load texture: data/setting.png" << std::endl;
        exit(1);
    }
    settingImgWidth = width;
    settingImgHeight = height;
    glGenTextures(1, &settingTexId);
    glBindTexture(GL_TEXTURE_2D, settingTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
}

// --- テクスチャの読み込み ---
void loadTexture() {
    int width, height, channels;
    unsigned char *data = stbi_load(TEX_FILE.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "Failed to load texture: " << TEX_FILE << std::endl;
        exit(1);
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

// ARTモードでのテクスチャ読み込み
void loadTextures() {
    for (int i = 0; i < 6; ++i) {
        int width, height, channels;
        unsigned char *data = stbi_load(TEX_FILES[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) {
            std::cerr << "Failed to load texture: " << TEX_FILES[i] << std::endl;
            exit(1);
        }
        glGenTextures(1, &textureIds[i]);
        glBindTexture(GL_TEXTURE_2D, textureIds[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
}

const int CYLINDER_SEGMENTS = 32;  // 円周の分割数
const float CYLINDER_RADIUS = 0.02f;
const float CYLINDER_LENGTH = 10.0f;

std::vector<float> genCylinderMesh_Xaxis() {
    std::vector<float> vertices;

    for (int i = 0; i <= CYLINDER_SEGMENTS; ++i) {
        float theta = 2.0f * M_PI * i / CYLINDER_SEGMENTS;
        float y = CYLINDER_RADIUS * cos(theta);
        float z = CYLINDER_RADIUS * sin(theta);

        // 2つの点: 始点(x=0), 終点(x=L)
        vertices.push_back(0.0f);            // x
        vertices.push_back(y);               // y
        vertices.push_back(z);               // z

        vertices.push_back(CYLINDER_LENGTH); // x
        vertices.push_back(y);               // y
        vertices.push_back(z);               // z
    }

    return vertices;
}



struct Cube {
    glm::mat4 transform;
    glm::ivec3 logicalPos;           // 論理位置 (x,y,z)
    glm::vec3 faceColors[6];         // 各面の色
};

Cube cubes[3][3][3];


// clang-format on

// バッファを参照する番号
// Indices for vertex/index buffers
GLuint vaoId;
GLuint vertexBufferId;
GLuint indexBufferId;
GLuint textureBufferId;

// シェーダプログラムを参照する番号
// Index for a shader program
GLuint programId;

// マウスドラッグ中かどうか
// Flag to check mouse is dragged or not
bool isDragging = false;

// マウスのクリック位置
// Mouse click position
glm::ivec2 oldPos;
glm::ivec2 newPos;

// 操作の種類
// Type of control
enum ArcballMode {
    ARCBALL_MODE_NONE = 0x00,
    ARCBALL_MODE_TRANSLATE = 0x01,
    ARCBALL_MODE_ROTATE = 0x02,
    ARCBALL_MODE_SCALE = 0x04
};

// 座標変換のための変数
// Variables for coordinate transformation
int arcballMode = ARCBALL_MODE_NONE;
glm::mat4 viewMat, projMat;
glm::mat4 acRotMat, acTransMat, acScaleMat;
float acScale = 1.0f;

// 立方体の回転角度
// Rotation angle for animating a cube
static float theta = 0.0f;

// オブジェクトを選択するためのID
// Index for identifying selected object
bool selectMode = false;

struct ArcballObject {
    glm::mat4 rotMat = glm::mat4(1.0f);
    glm::mat4 transMat = glm::mat4(1.0f);
    glm::mat4 scaleMat = glm::mat4(1.0f);
    float scale = 1.0f;
};


int selectedCube = 0;   // どちらが選択中か（0 or 1）
glm::mat4 globalRotMat = glm::mat4(1.0f);  // ルービックキューブ全体の回転行列


// 3x3x3のルービックキューブ構造（各小立方体の変換行列）
// 3x3x3 Rubik's cube: transformation matrix for each small cube
void initCubes() {
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                Cube& cube = cubes[x][y][z];
                cube.logicalPos = glm::ivec3(x, y, z);

                glm::vec3 offset = glm::vec3(x - 1, y - 1, z - 1) * 1.1f;
                cube.transform = glm::translate(glm::mat4(1.0f), offset);

                for (int f = 0; f < 6; ++f)
                    cube.faceColors[f] = colors[6];  // 黒で初期化

                // 外側の面だけ色を付ける
                if (x == 2) cube.faceColors[0] = colors[0]; // +X = 赤
                if (x == 0) cube.faceColors[5] = colors[3]; // -X = 黄
                if (y == 2) cube.faceColors[1] = colors[1]; // +Y = 緑
                if (y == 0) cube.faceColors[4] = colors[7]; // -Y = シアン
                if (z == 2) cube.faceColors[2] = colors[2]; // +Z = 青
                if (z == 0) cube.faceColors[3] = colors[5]; // -Z = マゼンタ
            }
        }
    }
}


void initRubikVAO() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    int idx = 0;

    if (ArtMode) {
        for (int x = 0; x < 3; ++x) {
            for (int y = 0; y < 3; ++y) {
                for (int z = 0; z < 3; ++z) {
                    for (int f = 0; f < 6; ++f) {
                        // 外側の面かどうか
                        bool isOuter =
                            (f == 0 && x == 2) || // +X
                            (f == 1 && y == 2) || // -X
                            (f == 2 && z == 2) || // +Y
                            (f == 3 && z == 0) || // -Y
                            (f == 4 && y == 0) || // +Z
                            (f == 5 && x == 0);   // -Z

                        // テクスチャ座標の計算（ArtMode用）
                        int i = 0, j = 0;
                        if (isOuter) {
                            switch (f) {
                                case 0: i = 2-z;     j = 2-y; break; // +X
                                case 1: i = x;     j = z;     break; // -X
                                case 2: i = x;     j = 2-y; break; // +Y
                                case 3: i = 2-x;     j = 2-y;     break; // -Y
                                case 4: i = x;     j = 2-z; break; // +Z
                                case 5: i = z;     j = 2-y;     break; // -Z
                            }
                        }
                        float u0 = isOuter ? i / 3.0f : 0.0f;
                        float v0 = isOuter ? j / 3.0f : 0.0f;
                        float u1 = isOuter ? (i + 1) / 3.0f : 0.0f;
                        float v1 = isOuter ? (j + 1) / 3.0f : 0.0f;

                        glm::vec2 texcoords[3] = {
                            glm::vec2(u0, v0), glm::vec2(u1, v0), glm::vec2(u1, v1)
                        };
                        glm::vec2 texcoords2[3] = {
                            glm::vec2(u0, v0), glm::vec2(u1, v1), glm::vec2(u0, v1)
                        };

                        // 2三角形×3頂点ずつ
                        for (int tri = 0; tri < 2; ++tri) {
                            for (int jv = 0; jv < 3; ++jv) {
                                int vidx = faces[f * 2 + tri][jv];
                                glm::vec2 tc = glm::vec2(0.0f);
                                if (isOuter) {
                                    if (tri == 0)      tc = texcoords[jv];
                                    else if (tri == 1) tc = texcoords2[jv];
                                }
                                Vertex v(positions[vidx], cubes[x][y][z].faceColors[f], tc);
                                vertices.push_back(v);
                                indices.push_back(idx++);
                            }
                        }
                    }
                }
            }
        }
    } else {
        // 3x3x3個の小立方体ごとにデータ生成
        for (int x = 0; x < 3; ++x) {
            for (int y = 0; y < 3; ++y) {
                for (int z = 0; z < 3; ++z) {
                    // 各面ごとに
                    for (int f = 0; f < 6; ++f) {
                        // テクスチャ座標
                        glm::vec2 texcoords[3] = {
                            glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1)
                        };
                        glm::vec2 texcoords2[3] = {
                            glm::vec2(0, 0), glm::vec2(1, 1), glm::vec2(0, 1)
                        };

                        bool isIconFace = (x == 1 && y == 0 && z == 1 && f == 4); // 白面中心か
                        
                        // 2三角形×3頂点ずつ
                        for (int j = 0; j < 3; ++j) {
                            glm::vec2 tc = glm::vec2(0.0f);
                            if (isIconFace) tc = texcoords[j];
                            Vertex v(positions[faces[f * 2 + 0][j]], cubes[x][y][z].faceColors[f], tc);
                            vertices.push_back(v);
                            indices.push_back(idx++);
                        }
                        for (int j = 0; j < 3; ++j) {
                            glm::vec2 tc = glm::vec2(0.0f);
                            if (isIconFace) tc = texcoords2[j];
                            Vertex v(positions[faces[f * 2 + 1][j]], cubes[x][y][z].faceColors[f], tc);
                            if (isIconFace) tc = texcoords[j];
                            vertices.push_back(v);
                            indices.push_back(idx++);
                        }
                    }
                }
            }
        }
    }

    // VAO/VBO/EBOの処理
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    glGenBuffers(1, &vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texcoord));

    glGenBuffers(1, &indexBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}
// 軸の円柱VAO

GLuint axisCylinderVao, axisCylinderVbo;

void initAxisCylinderVAO() {
    std::vector<float> vertices = genCylinderMesh_Xaxis();

    glGenVertexArrays(1, &axisCylinderVao);
    glGenBuffers(1, &axisCylinderVbo);

    glBindVertexArray(axisCylinderVao);
    glBindBuffer(GL_ARRAY_BUFFER, axisCylinderVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 頂点属性（位置）
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}


// シェーダのソースファイルをコンパイルする
// Compile a shader source
GLuint compileShader(const std::string &filename, GLuint type) {
    // シェーダの作成
    // Create a shader
    GLuint shaderId = glCreateShader(type);

    // ファイルの読み込み
    // Load source file
    std::ifstream reader;
    std::string code;

    // ファイルを開く
    // Open source file
    reader.open(filename.c_str(), std::ios::in);
    if (!reader.is_open()) {
        // ファイルを開けなかったらエラーを出して終了
        // Finish with error message if source file could not be opened
        fprintf(stderr, "Failed to load a shader: %s\n", filename.c_str());
        exit(1);
    }

    // ファイルをすべて読んで変数に格納
    // Load entire contents of a file and store to a string variable
    {
        // ファイル読み取り位置を終端に移動 / Move seek position to the end
        reader.seekg(0, std::ios::end);
        // コードを格納する変数の大きさを予約 / Reserve memory location for code characters
        code.reserve(reader.tellg());
        // ファイルの読み取り位置を先頭に移動 / Move seek position back to the beginning
        reader.seekg(0, std::ios::beg);

        // 先頭からファイルサイズ分を読んでコードの変数に格納
        // Load entire file and copy to "code" variable
        code.assign(std::istreambuf_iterator<char>(reader),
                    std::istreambuf_iterator<char>());
    }

    // ファイルを閉じる
    // Close file
    reader.close();

    // コードのコンパイル
    // Compile a source code
    const char *codeChars = code.c_str();
    glShaderSource(shaderId, 1, &codeChars, NULL);
    glCompileShader(shaderId);

    // コンパイルの成否を判定する
    // Check whther compile is successful
    GLint compileStatus;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        // コンパイルが失敗したらエラーメッセージとソースコードを表示して終了
        // Terminate with error message if compilation failed
        fprintf(stderr, "Failed to compile a shader!\n");

        // エラーメッセージの長さを取得する
        // Get length of error message
        GLint logLength;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            // エラーメッセージを取得する
            // Get error message
            GLsizei length;
            std::string errMsg;
            errMsg.resize(logLength);
            glGetShaderInfoLog(shaderId, logLength, &length, &errMsg[0]);

            // エラーメッセージとソースコードの出力
            // Print error message and corresponding source code
            fprintf(stderr, "[ ERROR ] %s\n", errMsg.c_str());
            fprintf(stderr, "%s\n", code.c_str());
        }
        exit(1);
    }

    return shaderId;
}

// シェーダプログラムのビルド (=コンパイル＋リンク)
// Build a shader program (build = compile + link)
GLuint buildShaderProgram(const std::string &vShaderFile, const std::string &fShaderFile) {
    // 各種シェーダのコンパイル
    // Compile shader files
    GLuint vertShaderId = compileShader(vShaderFile, GL_VERTEX_SHADER);
    GLuint fragShaderId = compileShader(fShaderFile, GL_FRAGMENT_SHADER);

    // シェーダプログラムへのリンク
    // Link shader objects to the program
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertShaderId);
    glAttachShader(programId, fragShaderId);
    glLinkProgram(programId);

    // リンクの成否を判定する
    // Check whether link is successful
    GLint linkState;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkState);
    if (linkState == GL_FALSE) {
        // リンクに失敗したらエラーメッセージを表示して終了
        // Terminate with error message if link is failed
        fprintf(stderr, "Failed to link shaders!\n");

        // エラーメッセージの長さを取得する
        // Get length of error message
        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            // エラーメッセージを取得する
            // Get error message
            GLsizei length;
            std::string errMsg;
            errMsg.resize(logLength);
            glGetProgramInfoLog(programId, logLength, &length, &errMsg[0]);

            // エラーメッセージを出力する
            // Print error message
            fprintf(stderr, "[ ERROR ] %s\n", errMsg.c_str());
        }
        exit(1);
    }

    // シェーダを無効化した後にIDを返す
    // Disable shader program and return its ID
    glUseProgram(0);
    return programId;
}

// シェーダの初期化
// Initialization related to shader programs
void initShaders() {
    programId = buildShaderProgram(VERT_SHADER_FILE, FRAG_SHADER_FILE);
}

// ユーザ定義のOpenGLの初期化
// User-define OpenGL initialization
void initializeGL() {
    // 深度テストの有効化
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // 背景色の設定 (黒)
    // Background color (black)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    initCubes();

    loadSettingTexture();
    if (ArtMode) {
        // ARTモードではテクスチャを読み込む
        // Load textures in ART mode
        loadTextures();
    } else {
        // 通常モードでは単一のテクスチャを読み込む
        // Load a single texture in normal mode
        loadTexture();
    }

    //initCubeTransforms();  // 3x3x3の小立方体の変換行列を初期化
    // VAOの初期化
    // Initialize VAO
    initRubikVAO();
 
    // 軸の円柱VAOの初期化
    initAxisCylinderVAO();

    // シェーダの用意
    // Prepare shader program
    initShaders();

    // カメラの姿勢を決定する変換行列の初期化
    // Initialize transformation matrices for camera pose
    projMat = glm::perspective(glm::radians(45.0f), (float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 1000.0f);

    viewMat = glm::lookAt(glm::vec3(3.0f, 4.0f, 5.0f),   // 視点の位置 / Eye position
                          glm::vec3(0.0f, 0.0f, 0.0f),   // 見ている先 / Looking position
                          glm::vec3(0.0f, 1.0f, 0.0f));  // 視界の上方向 / Upward vector

    // アークボール操作のための変換行列を初期化
    // Initialize transformation matrices for arcball control
    acRotMat = glm::mat4(1.0);
    acTransMat = glm::mat4(1.0);
    acScaleMat = glm::mat4(1.0);

}


// 回転対象の軸とスライス番号を指定
// Axis: 0=x, 1=y, 2=z
int selectedAxis = 0;
int selectedIndex = 0;

// 90度回転後の座標を取得する関数
std::pair<int, int> getLogicalPos(int i, int j, bool clockwise) {
    if (clockwise) 
        return {2 - j, i};  // 90度回転後の座標（2D）
    else
        return {j, 2 - i};  // -90度回転後の座標（2D）
}
std::pair<int, int> getLogicalPosoIverse(int i, int j) {
    return {2 -i, 2 - j};  // 90度回転後の座標（2D）
}

std::vector<glm::ivec3> targets;
glm::mat4 originalTransforms[3][3][3];
glm::vec3 center(0.0f);
bool clockwise = true;

void applyRotation(int axis, int index, float angleStep, float rotationAngle, bool rotating_in) {
    bool is90Final = rotating_in;

    glm::vec3 axisVec = (axis == 0) ? glm::vec3(1, 0, 0)
                     : (axis == 1) ? glm::vec3(0, 1, 0)
                                   : glm::vec3(0, 0, 1);
    
    if (rotating_in) {
        // アニメーション開始時に保存
        if (rotationAngle == angleStep) {
            for (int x = 0; x < 3; ++x)
            for (int y = 0; y < 3; ++y)
                for (int z = 0; z < 3; ++z) {
                    const Cube& cube = cubes[x][y][z];
                    glm::ivec3 p = cube.logicalPos;
                    if ((axis == 0 && p.x == index) ||
                        (axis == 1 && p.y == index) ||
                        (axis == 2 && p.z == index)) {
                        targets.emplace_back(x, y, z);
                        center += glm::vec3(p - glm::ivec3(1)) * 1.1f;
                    }
                }

            center /= (float)targets.size();
            for (const auto& idx : targets)
                originalTransforms[idx.x][idx.y][idx.z] = cubes[idx.x][idx.y][idx.z].transform;
        }

        glm::mat4 M = glm::rotate(glm::radians(rotationAngle), axisVec);

        for (const auto& idx : targets)
            cubes[idx.x][idx.y][idx.z].transform = M * originalTransforms[idx.x][idx.y][idx.z];

        return;
    } else {
        for (const auto& idx : targets) {
            const Cube& cube_1 = cubes[idx.x][idx.y][idx.z];
            glm::ivec3 p_1 = cube_1.logicalPos;
            if (axis == 0) {
                auto [ni, nj] = getLogicalPos(p_1.y, p_1.z, clockwise);  // 90度回転後の座標（2D）
                glm::ivec3 newPos;
                // printf("idx: %d, %d, %d, OldPos: %d, %d, %d-> %d, %d, %d, rotationAngle: %f\n", idx.x, idx.y, idx.z, p_1.x, p_1.y, p_1.z, index, ni, nj, rotationAngle);
                newPos = glm::ivec3(index, ni, nj);
                cubes[idx.x][idx.y][idx.z].logicalPos = newPos;
            } else if (axis == 1) {
                auto [ni, nj] = getLogicalPos(p_1.z, p_1.x, clockwise);  // 90度回転後の座標（2D）
                // printf("idx: %d, %d, %d, OldPos: %d, %d, %d-> %d, %d, %d, rotationAngle: %f\n", idx.x, idx.y, idx.z, p_1.x, p_1.y, p_1.z, nj, index, ni,rotationAngle);
                glm::ivec3 newPos;
                newPos = glm::ivec3(nj, index, ni);
                cubes[idx.x][idx.y][idx.z].logicalPos = newPos;
            } else if (axis == 2) {
                auto [ni, nj] = getLogicalPos(p_1.x, p_1.y, clockwise);  // 90度回転後の座標（2D）
                glm::ivec3 newPos;
                newPos = glm::ivec3(ni, nj, index);
                cubes[idx.x][idx.y][idx.z].logicalPos = newPos;
            }
        targets.clear();
        }
    }

}


// ユーザ定義のOpenGL描画
// User-defined OpenGL drawing
bool selectingMode = true; // ←追加: モード選択中かどうか
bool AxisVisible = true; // 軸の表示切替

void paintGL() {
    // 背景色と深度バッファのクリア
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // シェーダプログラムの有効化
    glUseProgram(programId);

    // VAOのバインド
    glBindVertexArray(vaoId);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    // シェーダのサンプラーユニフォームに0をセット
    glUniform1i(glGetUniformLocation(programId, "u_sampler"), 0);

    struct SimpleVertex {
        glm::vec2 pos;
        glm::vec2 uv;
    };

    if (selectingMode) {
        // 2D用の直交投影行列をセット
        glm::mat4 ortho = glm::ortho(0.0f, (float)WIN_WIDTH, 0.0f, (float)WIN_HEIGHT);
        glUniformMatrix4fv(glGetUniformLocation(programId, "u_mvpMat"), 1, GL_FALSE, glm::value_ptr(ortho));
        glUniform1i(glGetUniformLocation(programId, "u_mode"), 2);

        // setting.pngをバインド
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, settingTexId);
        glUniform1i(glGetUniformLocation(programId, "u_sampler"), 0);

        int imgWidth = WIN_WIDTH;
        int imgHeight = WIN_HEIGHT;
        float imgAspect = (float)settingImgWidth / settingImgHeight;
        float winAspect = (float)WIN_WIDTH / WIN_HEIGHT;

        float drawWidth, drawHeight;
        if (winAspect > imgAspect) {
            // ウィンドウが横長 → 高さ基準
            drawHeight = WIN_HEIGHT;
            drawWidth = imgAspect * drawHeight;
        } else {
            // ウィンドウが縦長 → 幅基準
            drawWidth = WIN_WIDTH;
            drawHeight = drawWidth / imgAspect;
        }
        float cx = WIN_WIDTH / 2.0f, cy = WIN_HEIGHT / 2.0f;
        float x0 = cx - drawWidth / 2, x1 = cx + drawWidth / 2;
        float y0 = cy - drawHeight / 2, y1 = cy + drawHeight / 2;
        SimpleVertex quad[6] = {
            { {x0, y0}, {0, 1} }, { {x1, y0}, {1, 1} }, { {x1, y1}, {1, 0} },
            { {x0, y0}, {0, 1} }, { {x1, y1}, {1, 0} }, { {x0, y1}, {0, 0} }
        };

        struct SimpleVertex3D { glm::vec3 pos; glm::vec2 uv; };
        SimpleVertex3D quad3d[6];
        for (int i = 0; i < 6; ++i) {
            quad3d[i].pos = glm::vec3(quad[i].pos, 0.0f);
            quad3d[i].uv = quad[i].uv;
        }

        GLuint tmpVao, tmpVbo;
        glGenVertexArrays(1, &tmpVao);
        glGenBuffers(1, &tmpVbo);
        glBindVertexArray(tmpVao);
        glBindBuffer(GL_ARRAY_BUFFER, tmpVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad3d), quad3d, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex3D), (void*)0);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex3D), (void*)(sizeof(glm::vec3)));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDeleteBuffers(1, &tmpVbo);
        glDeleteVertexArrays(1, &tmpVao);

        return;
    }

    // 3×3×3の小立方体を描画
    // 各立方体ごとに36インデックスずつずらして描画
    if (ArtMode) {
        // 3x3x3個の小立方体ごとにデータ生成
        int cubeIndex = 0;
        for (int x = 0; x < 3; ++x) {
            for (int y = 0; y < 3; ++y) {
                for (int z = 0; z < 3; ++z) {
                    Cube& cube = cubes[x][y][z];
                    glm::mat4 modelMat = acTransMat * globalRotMat * acRotMat * acScaleMat * cube.transform;
                    modelMat = glm::scale(modelMat, glm::vec3(0.5f));
                    glm::mat4 mvpMat = projMat * viewMat * modelMat;

                    GLuint mvpMatLocId = glGetUniformLocation(programId, "u_mvpMat");
                    glUniformMatrix4fv(mvpMatLocId, 1, GL_FALSE, glm::value_ptr(mvpMat));
                    glUniform1i(glGetUniformLocation(programId, "u_selectID"), -1);
                    glUniform1i(glGetUniformLocation(programId, "object"), 1);

                    // 各面ごとに対応するテクスチャをバインドして描画
                    for (int f = 0; f < 6; ++f) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, textureIds[f]);
                        glUniform1i(glGetUniformLocation(programId, "u_sampler"), 0);
                        glUniform1i(glGetUniformLocation(programId, "u_mode"), 1);

                        // 1面=2三角形=6頂点
                        int faceStart = (cubeIndex * 36) + (f * 6);
                        glDrawElements(
                            GL_TRIANGLES,
                            6,
                            GL_UNSIGNED_INT,
                            (void*)(sizeof(unsigned int) * faceStart)
                        );
                    }
                    ++cubeIndex;
                }
            }
        }
    } else {
        int cubeIndex = 0;
        for (int x = 0; x < 3; ++x) {
            for (int y = 0; y < 3; ++y) {
                for (int z = 0; z < 3; ++z) {
                    // glm::mat4 modelMat = acTransMat * acRotMat * acScaleMat * cubeTransforms[x][y][z];
                    // modelMat = glm::scale(modelMat, glm::vec3(0.5f));
                    // glm::mat4 mvpMat = projMat * viewMat * modelMat;
                    Cube& cube = cubes[x][y][z];
                    glm::mat4 modelMat = acTransMat * globalRotMat * acRotMat * acScaleMat * cube.transform;
                    modelMat = glm::scale(modelMat, glm::vec3(0.5f));
                    glm::mat4 mvpMat = projMat * viewMat * modelMat;




                    GLuint mvpMatLocId = glGetUniformLocation(programId, "u_mvpMat");
                    glUniformMatrix4fv(mvpMatLocId, 1, GL_FALSE, glm::value_ptr(mvpMat));

                    GLuint uid = glGetUniformLocation(programId, "u_selectID");
                    glUniform1i(uid, -1);
                    glUniform1i(glGetUniformLocation(programId, "object"), 1);
                    glUniform1i(glGetUniformLocation(programId, "u_mode"), 0);

                    // ここでインデックスバッファのオフセットを指定
                    glDrawElements(
                        GL_TRIANGLES,
                        36,
                        GL_UNSIGNED_INT,
                        (void*)(sizeof(unsigned int) * 36 * cubeIndex)
                    );
                    ++cubeIndex;
                }
            }
        }
    }


    // 軸の色
    if (!AxisVisible) {
        // 軸の描画をスキップ
        glBindVertexArray(0);
        glUseProgram(0);
        return;
    }
    for (int i = 0; i < 3; ++i) {
        glm::vec3 axisColor;
        glm::mat4 model = glm::mat4(1.0f);
        if (i == 0) {
            axisColor = glm::vec3(1, 0, 0); // 赤（X軸）
            // model = cubes[2][1][1].transform * glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0));
            model = glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0)) * glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0, 0));
        } else if (i == 1) {
            axisColor = glm::vec3(0, 1, 0); // 緑（Y軸）
            // model = cubes[1][2][1].transform * glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1));
            model = glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1)) * glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0, 0));
        } else if (i == 2) {
            axisColor = glm::vec3(0, 0, 1); // 青（Z軸）
            // model = cubes[1][1][2].transform * glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
            model = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0)) * glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0, 0));
        }
        // モデル変換（X軸方向なので不要）
        
        glm::mat4 mvp = projMat * viewMat * acTransMat * globalRotMat * acScaleMat * model;

        glUniformMatrix4fv(glGetUniformLocation(programId, "u_mvpMat"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3fv(glGetUniformLocation(programId, "u_color"), 1, glm::value_ptr(axisColor));
        glUniform1i(glGetUniformLocation(programId, "object"), 0);

        glBindVertexArray(axisCylinderVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, CYLINDER_SEGMENTS * 2 + 2);
    }
    


    // VAOのアンバインド
    glBindVertexArray(0);

    // シェーダ無効化
    glUseProgram(0);
}


// ウィンドウサイズ変更のコールバック関数
// Callback function for window resizing
void resizeGL(GLFWwindow *window, int width, int height) {
    // ユーザ管理のウィンドウサイズを変更
    // Update user-managed window size
    WIN_WIDTH = width;
    WIN_HEIGHT = height;

    // GLFW管理のウィンドウサイズを変更
    // Update GLFW-managed window size
    glfwSetWindowSize(window, WIN_WIDTH, WIN_HEIGHT);

    // 実際のウィンドウサイズ (ピクセル数) を取得
    // Get actual window size by pixels
    int renderBufferWidth, renderBufferHeight;
    glfwGetFramebufferSize(window, &renderBufferWidth, &renderBufferHeight);

    // ビューポート変換の更新
    // Update viewport transform
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);

    // 東映変換行列の更新
    // Update projection matrix
    projMat = glm::perspective(glm::radians(45.0f), (float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 1000.0f);
}

// マウスのクリックを処理するコールバック関数
// Callback for mouse click events
void mouseEvent(GLFWwindow *window, int button, int action, int mods) {

    double px, py;
    glfwGetCursorPos(window, &px, &py);
    const int cx = (int)px;
    const int cy = (int)py;

    if (selectingMode && action == GLFW_PRESS) {
        // モード選択中のクリック処理
        double px, py;
        glfwGetCursorPos(window, &px, &py);
        if (px < WIN_WIDTH / 2 && py > WIN_HEIGHT / 9 * 7 && py < WIN_HEIGHT / 9 * 8) {
            ArtMode = false;
            selectingMode = false;
        } else if (px >= WIN_WIDTH / 2 && py > WIN_HEIGHT / 9 * 7 && py < WIN_HEIGHT / 9 * 8) {
            ArtMode = true;
            selectingMode = false;
        }
        
        // 再初期化
        initializeGL();
        return;
    }

    if (action == GLFW_PRESS) {
        // クリック位置で選択判定
        // 選択モードでの描画
        selectMode = true;
        paintGL();
        selectMode = false;

        // ピクセルの大きさの計算 (Macの場合には必要)
        int renderBufferWidth, renderBufferHeight;
        glfwGetFramebufferSize(window, &renderBufferWidth, &renderBufferHeight);
        int pixelSize = std::max(renderBufferWidth / WIN_WIDTH, renderBufferHeight / WIN_HEIGHT);

        // より適切なやり方 (1ピクセルだけを読む)
        unsigned char byte[4];
        glReadPixels(cx * pixelSize, (WIN_HEIGHT - cy - 1) * pixelSize, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, byte);
        // printf("Select object %d\n", (int)byte[0]);

        // ここでドラッグ開始
        isDragging = true;
        oldPos = glm::ivec2(px, py);
        newPos = glm::ivec2(px, py);

        // 操作モードの設定
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            arcballMode = ARCBALL_MODE_ROTATE;
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            arcballMode = ARCBALL_MODE_SCALE;
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            arcballMode = ARCBALL_MODE_TRANSLATE;
        }
    } else if (action == GLFW_RELEASE) {
        isDragging = false;
        oldPos = glm::ivec2(0, 0);
        newPos = glm::ivec2(0, 0);
        arcballMode = ARCBALL_MODE_NONE;
    }
}

// スクリーン上の位置をアークボール球上の位置に変換する関数
// Convert screen-space coordinates to a position on the arcball sphere
glm::vec3 getVector(double x, double y) {
    // 円がスクリーンの長辺に内接していると仮定
    // Assume a circle contacts internally with longer edges
    const int shortSide = std::min(WIN_WIDTH, WIN_HEIGHT);
    glm::vec3 pt(2.0f * x / (float)shortSide - 1.0f, -2.0f * y / (float)shortSide + 1.0f, 0.0f);

    // z座標の計算
    // Calculate Z coordinate
    const double xySquared = pt.x * pt.x + pt.y * pt.y;
    if (xySquared <= 1.0) {
        // 単位円の内側ならz座標を計算
        // Calculate Z coordinate if a point is inside a unit circle
        pt.z = std::sqrt(1.0 - xySquared);
    } else {
        // 外側なら球の外枠上にあると考える
        // Suppose a point is on the circle line if the click position is outside the unit circle
        pt = glm::normalize(pt);
    }

    return pt;
}

// 回転成分の更新
// Update rotation matrix
void updateRotate() {
    const glm::vec3 u = getVector(oldPos.x, oldPos.y);
    const glm::vec3 v = getVector(newPos.x, newPos.y);

    if (u == v) return;

    const double angle = std::acos(std::clamp(glm::dot(u, v), -1.0f, 1.0f));
    const glm::vec3 rotAxis = glm::cross(u, v);
    if (glm::length(rotAxis) < 1e-5) return;

    glm::mat4 c2wMat = glm::inverse(viewMat);
    glm::vec3 rotAxisWorld = glm::vec3(c2wMat * glm::vec4(rotAxis, 0.0f));

    // 回転行列をグローバルに適用
    globalRotMat = glm::rotate((float)(2.0 * angle), rotAxisWorld) * globalRotMat;
}


// 平行移動量成分の更新 (実装しない)
// Update translation matrix
// void updateTranslate() {
//     // NOTE:
//     // この関数では物体が世界座標の原点付近にあるとして平行移動量を計算する
//     // This function assumes the object locates near to the world-space origin and computes the amount of translation

//     // 世界座標の原点のスクリーン座標を求める
//     // Calculate screen-space coordinates of the world-space origin
//     glm::vec4 originScreenSpace = (projMat * viewMat) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
//     originScreenSpace /= originScreenSpace.w;

//     // スクリーン座標系におけるマウス移動の視点と終点の計算. これらの位置はスクリーン座標系のZ座標に依存することに注意する
//     // Calculate the start and end points of mouse motion, which depend Z coordinate in screen space
//     glm::vec4 newPosScreenSpace(2.0f * newPos.x / WIN_WIDTH - 1.0f, -2.0f * newPos.y / WIN_HEIGHT + 1.0f, originScreenSpace.z, 1.0f);
//     glm::vec4 oldPosScreenSpace(2.0f * oldPos.x / WIN_WIDTH - 1.0f, -2.0f * oldPos.y / WIN_HEIGHT + 1.0f, originScreenSpace.z, 1.0f);

//     // スクリーン座標の情報を世界座標座標に変換する行列 (= MVP行列の逆行列)
//     // Transformation from screen space to world space (= inverse of MVP matrix)
//     glm::mat4 invMvpMat = glm::inverse(projMat * viewMat);

//     // スクリーン空間の座標を世界座標に変換
//     // Transform screen-space positions to world-space positions
//     glm::vec4 newPosObjSpace = invMvpMat * newPosScreenSpace;
//     glm::vec4 oldPosObjSpace = invMvpMat * oldPosScreenSpace;
//     newPosObjSpace /= newPosObjSpace.w;
//     oldPosObjSpace /= oldPosObjSpace.w;

//     // 世界座標系で移動量を求める
//     // Calculate the amount of translation in world space
//     const glm::vec3 transWorldSpace = glm::vec3(newPosObjSpace - oldPosObjSpace);

//     // 行列に変換
//     // Calculate translation matrix
//     // cubes[selectedCube].transMat = glm::translate(transWorldSpace) * cubes[selectedCube].transMat;
// }

// 物体の拡大縮小率を更新 (実装しない)
// Update object scale
// void updateScale() {
//     //cubes[selectedCube].scaleMat = glm::scale(glm::vec3(cubes[selectedCube].scale, cubes[selectedCube].scale, cubes[selectedCube].scale));
// }

// 変換行列の更新. マウス操作の内容に応じて更新対象を切り替える
// Update transformation matrices, depending on type of mouse interaction
void updateTransform() {
    switch (arcballMode) {
    case ARCBALL_MODE_ROTATE:
        updateRotate();
        break;

    case ARCBALL_MODE_TRANSLATE:
        // updateTranslate();
        break;

    case ARCBALL_MODE_SCALE:
        acScale += (float)(oldPos.y - newPos.y) / WIN_HEIGHT;
        // updateScale();
        break;
    }
}

// マウスの動きを処理するコールバック関数
// Callback for mouse move events
void motionEvent(GLFWwindow *window, double xpos, double ypos) {
    if (isDragging) {
        // マウスの現在位置を更新
        // Update current mouse position
        newPos = glm::ivec2(xpos, ypos);

        // マウスがあまり動いていない時は処理をしない
        // Update transform only when mouse moves sufficiently
        const double dx = newPos.x - oldPos.x;
        const double dy = newPos.y - oldPos.y;
        const double length = dx * dx + dy * dy;
        if (length < 2.0 * 2.0) {
            return;
        } else {
            updateTransform();
            oldPos = glm::ivec2(xpos, ypos);
        }
    }
}

// マウスホイールを処理するコールバック関数 (実装しない)
// Callback for mouse wheel event
// void wheelEvent(GLFWwindow *window, double xoffset, double yoffset) {
//     acScale += yoffset / 10.0;
//     // updateScale();
// }

bool rotating = false;
float rotationAngle = 0.0f;
// Shuffle operations
// グローバル
struct ShuffleMove {
    int axis;
    int index;
    bool clockwise;
};
std::vector<ShuffleMove> shuffleMoves;
bool isShuffling = false;

void startShuffle(int numMoves = 30) {
    shuffleMoves.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> axisDist(0, 2);
    std::uniform_int_distribution<> indexDist(0, 2);
    std::uniform_int_distribution<> dirDist(0, 1);

    for (int i = 0; i < numMoves; ++i) {
        ShuffleMove move;
        move.axis = axisDist(gen);
        move.index = indexDist(gen);
        move.clockwise = dirDist(gen) == 0;
        shuffleMoves.push_back(move);
    }

    isShuffling = true;
    rotating = false; // すぐ1手目を開始できるように
}

bool clockwise_w = true; // Wキーの状態を管理

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    if (action == GLFW_PRESS && key == GLFW_KEY_S && mods && GLFW_MOD_CONTROL) {
        // Ctrl + S でシャッフル開始
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(25, 35);
        int random_num = dist(gen);
        startShuffle(random_num); // 25~35手シャッフル
        // printf("Shuffling started with %d moves.\n", random_num);
        printf("Shuffling starts!\n");
    }


    // Wキーの押下・離上でclockwiseを切り替え
    if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) {
            clockwise_w = false;
        } else if (action == GLFW_RELEASE) {
            // printf("W key released, clockwise_w set to true.\n");
            clockwise_w = true;
        }
    }
    
    if (action == GLFW_PRESS && !rotating) {

        if (mods & GLFW_MOD_SUPER) {
            // Ctrlキーが押されている場合は回転を開始
            selectedIndex= 0;
        } else {
            // Ctrlキーが押されていない場合は回転を停止
            selectedIndex = 2;
        }

        if (mods & GLFW_MOD_ALT || (GLFW_PRESS && key == GLFW_KEY_S && mods && GLFW_MOD_CONTROL)) {
            // Altキーが押されている時とshuffle時は非表示
            AxisVisible = false;
        } else {
            AxisVisible = true;
        }

        if (key == GLFW_KEY_B) {
            selectedAxis = 2;        // Z軸
            // selectedIndex = 2;       // z=0 面
            rotating = true;
            rotationAngle = 0.0f;
            clockwise = clockwise_w;  // 時計回り
        } else if (key == GLFW_KEY_V) {
            selectedAxis = 2;
            selectedIndex = 1;
            rotating = true;
            rotationAngle = 0.0f;
            clockwise = clockwise_w;
        }
        // 必要に応じて他のキーにも追加（x, y 軸用など）
        else if (key == GLFW_KEY_G) {
            selectedAxis = 1;
            // selectedIndex = 2;
            rotating = true;
            rotationAngle = 0.0f;
            clockwise = clockwise_w;  // 時計回り
        } else if (key == GLFW_KEY_F) {
            selectedAxis = 1;
            selectedIndex = 1;
            rotating = true;
            rotationAngle = 0.0f;
            clockwise = clockwise_w;  // 時計回り
        } else if (key == GLFW_KEY_R) {
            selectedAxis = 0;
            // selectedIndex = 2;
            rotating = true;
            rotationAngle = 0.0f;
            clockwise = clockwise_w;  // 時計回り
        } else if (key == GLFW_KEY_E) {
            selectedAxis = 0;
            selectedIndex = 1;
            rotating = true;
            rotationAngle = 0.0f;
            clockwise = clockwise_w;  // 時計回り
        }
    }
}



void update() {
    if (isShuffling) {
        if (!rotating && !shuffleMoves.empty()) {
            // 次の手を開始
            ShuffleMove move = shuffleMoves.front();
            shuffleMoves.erase(shuffleMoves.begin());
            selectedAxis = move.axis;
            selectedIndex = move.index;
            clockwise = move.clockwise;
            rotationAngle = 0.0f;
            rotating = true;
        }
        if (rotating) {
            float angleStep = clockwise ? 3.0f : -3.0f;
            rotationAngle += angleStep;
            if ((clockwise && rotationAngle > 90.0f) || (!clockwise && rotationAngle < -90.0f)) {
                rotating = false;
            }
            applyRotation(selectedAxis, selectedIndex, angleStep, rotationAngle, rotating);
        }
        // シャッフル終了
        if (!rotating && shuffleMoves.empty()) {
            isShuffling = false;
            AxisVisible = true; // シャッフル終了時に軸を表示
            std::cout << "Shuffle completed." << std::endl;
        }
        return;
    }

    if (!rotating) return;
    float angleStep = 0.0f;

    if (clockwise) {
        angleStep = 3.0f;
        
        rotationAngle += angleStep;
        // std::cout << "Rotation angle: " << rotationAngle << std::endl;

        if (rotationAngle > 90.0f) {
            // angleStep -= (rotationAngle - 90.0f);  // 最後のステップ調整
            rotating = false;

        }
    } else {
        angleStep = -3.0f;
        
        rotationAngle += angleStep;
        // std::cout << "Rotation angle: " << rotationAngle << std::endl;

        if (rotationAngle < -90.0f) {
            // angleStep -= (rotationAngle + 90.0f);  // 最後のステップ調整
            rotating = false;
        }
    }
    // std::cout << "Rotating around axis: " << selectedAxis << ", index: " << selectedIndex << ", angle: " << rotationAngle << std::endl;

    applyRotation(selectedAxis, selectedIndex, angleStep, rotationAngle, rotating);
}


// アニメーションのためのアップデート
// Update parameters for animation
void animate() {
    theta += 1.0f;  // 1度だけ回転 / Rotate 1 degree of angle
}

int main(int argc, char **argv) {
    // OpenGLを初期化する
    // OpenGL initialization
    if (glfwInit() == GLFW_FALSE) {
        fprintf(stderr, "Initialization failed!\n");
        return 1;
    }

    // OpenGLのバージョン設定 (Macの場合には必ず必要)
    // Specify OpenGL version (mandatory for Mac)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Windowの作成
    // Create a window
    GLFWwindow *window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE,
                                          NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        fprintf(stderr, "Window creation failed!\n");
        return 1;
    }

    // OpenGLの描画対象にwindowを指定
    // Specify window as an OpenGL context
    glfwMakeContextCurrent(window);

    // OpenGL 3.x/4.xの関数をロードする (glfwMakeContextCurrentの後でないといけない)
    // Load OpenGL 3.x/4.x methods (must be loaded after "glfwMakeContextCurrent")
    const int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        fprintf(stderr, "Failed to load OpenGL 3.x/4.x libraries!\n");
        return 1;
    }

    // バージョンを出力する / Check OpenGL version
    printf("Load OpenGL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    // ウィンドウのリサイズを扱う関数の登録
    // Register a callback function for window resizing
    glfwSetWindowSizeCallback(window, resizeGL);

    // マウスのイベントを処理する関数を登録
    // Register a callback function for mouse click events
    glfwSetMouseButtonCallback(window, mouseEvent);

    // マウスの動きを処理する関数を登録
    // Register a callback function for mouse move events
    glfwSetCursorPosCallback(window, motionEvent);

    // マウスホイールを処理する関数を登録(実装しない)
    // Register a callback function for mouse wheel
    // glfwSetScrollCallback(window, wheelEvent);

    glfwSetKeyCallback(window, key_callback);


    // ユーザ指定の初期化
    // User-specified initialization
    initializeGL();

    selectingMode = true; // ←追加

    // メインループ
    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        update();  // アニメーションの更新
        // 描画 / Draw
        paintGL();

        // アニメーション / Animation
        animate();

        // 描画用バッファの切り替え
        // Swap drawing target buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 後処理 / Postprocess
    glfwDestroyWindow(window);
    glfwTerminate();
}