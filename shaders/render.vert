#version 330

// Attribute変数
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_texcoord;

// Varying変数
out vec3 f_fragColor;
out vec2 f_texcoord; 

// Uniform変数
uniform mat4 u_mvpMat;
uniform int u_mode; // 0:通常, 1:ArtMode, 2:2D画像描画

void main() {
    if (u_mode == 2) {
        // 2D画像描画用: in_positionのx,yのみ使う
        gl_Position = u_mvpMat * vec4(in_position.xy, 0.0, 1.0);
        f_fragColor = vec3(1.0);
        f_texcoord = in_texcoord;
    } else {
        // gl_Positionは頂点シェーダの組み込み変数
        // 指定を忘れるとエラーになるので注意
        gl_Position = u_mvpMat * vec4(in_position, 1.0);

        // Varying変数への代入
        f_fragColor = in_color;

        f_texcoord = in_texcoord;               // 追加
    }
}