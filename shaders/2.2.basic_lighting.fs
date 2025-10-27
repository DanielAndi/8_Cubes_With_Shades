#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float shininess;

void main()
{
    // --- Light attenuation ---
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.02 * distance + 0.001 * distance * distance);

    // --- Ambient ---
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    // --- Diffuse ---
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // --- Specular ---
    float specularStrength = 1.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // --- Combine (keep specular stronger at distance) ---
    vec3 result = ((ambient + diffuse) * attenuation + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
