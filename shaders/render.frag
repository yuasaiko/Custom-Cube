#version 330

// Varying変数
uniform int u_selectID;
uniform int object; 
uniform vec3 u_color;
uniform int u_mode;         // ← 追加: 0=通常, 1=ArtMode
uniform sampler2D u_sampler;

in vec3 f_fragColor;
in vec2 f_texcoord; // 頂点シェーダから受け取る

// ディスプレイへの出力変数
out vec4 out_color;


void main() {
    if (u_mode == 2) {
        out_color = texture(u_sampler, f_texcoord);
    } else if (u_selectID > 0) {
        out_color = vec4(float(u_selectID) / 255.0, 0.0, 0.0, 1.0);
    } else if (object == 0) {
        out_color = vec4(u_color, 1.0);      // 軸や円柱
    } else if (u_mode == 1 && length(f_texcoord) > 0.0) {
        // ArtMode: use glGenTextures
        out_color = texture(u_sampler, f_texcoord);
    } else if (u_mode == 0 && length(f_texcoord) > 0.0) {
        // NonalMode: use glGenTextures
        out_color = texture(u_sampler, f_texcoord);
    } else {
        out_color = vec4(f_fragColor, 1.0);  // キューブ
    }
}