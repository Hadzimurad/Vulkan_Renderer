#version 450 core

// Выходной вектор (RGBA цвет пикселя)
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 FragColor;

void main()
{
    // Задаем сплошной оранжевый цвет (Красный=1.0, Зеленый=0.5, Синий=0.2, Альфа=1.0)
    FragColor = vec4(fragColor, 1.0);
}

