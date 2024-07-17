/* #version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in mat4 instance_transform;

uniform vec3 chunk_pos;
uniform mat4 mvp; // view projection matrix

out vec4 frag_col;

void main()
{
	frag_col = vec4(0.7, 0.0, 0.3, 1.0);
	// gl_Position = mvp * (vec4(chunk_pos, 1.0) + (vec4(vertex_pos, 1.0) * instance_transform));
	gl_Position = mvp * vec4(vertex_pos, 1.0)
} */

#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
//in vec4 vertexColor;      // Not required

in mat4 instanceTransform;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

// NOTE: Add here your custom variables

void main()
{
    // Send vertex attributes to fragment shader
    fragPosition = vec3(instanceTransform*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    //fragColor = vertexColor;
    // fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));
    // fragNormal = normalize(vec3(instanceTransform)*vec3(matNormal*vec4(vertexNormal, 1.0)));
	// fragNormal = mat3(transpose(inverse(mvp * vec3(instanceTransform)))) * vertexNormal;
	// fragNormal = mat3(transpose(inverse(mvp * instanceTransform))) * vertexNormal;
	fragNormal = vec3(mvp*instanceTransform*vec4(vertexNormal, 1.0));

    // Calculate final vertex position, note that we multiply mvp by instanceTransform
    gl_Position = mvp*instanceTransform*vec4(vertexPosition, 1.0);
}
