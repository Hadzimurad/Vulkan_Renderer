#version 450

// Блок push-констант. Данные обновляются мгновенно через регистры GPU
layout(push_constant) uniform Constants {
    mat4 model;
    mat4 proj;
    mat4 view;
} push;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    // Умножаем матрицу на позицию вершины (Z = 0.0, W = 1.0)
    gl_Position = push.proj * push.view * push.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
