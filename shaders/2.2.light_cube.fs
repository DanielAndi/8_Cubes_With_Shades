#version 330 core
out vec4 FragColor;

void main()
{
    // Brighter visible cube (not affecting actual lighting)
    FragColor = vec4(vec3(1.0), 1.0); // 3× brighter white
}
