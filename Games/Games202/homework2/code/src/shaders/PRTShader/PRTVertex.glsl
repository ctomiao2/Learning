attribute vec3 aVertexPosition;
attribute vec3 aNormalPosition;
attribute vec2 aTextureCoord;
attribute mat3 aPrecomputeLT;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uLightMVP;
uniform sampler2D uSampler;
uniform mat3 uPrecomputeLR;
uniform mat3 uPrecomputeLG;
uniform mat3 uPrecomputeLB;

varying highp vec2 vTextureCoord;
varying highp vec3 vFragPos;
varying highp vec3 vNormal;
varying highp vec4 vPositionFromLight;
varying highp vec3 vColor;

#define PI 3.141592653589793

void main(void) {

  vFragPos = (uModelMatrix * vec4(aVertexPosition, 1.0)).xyz;
  vNormal = (uModelMatrix * vec4(aNormalPosition, 0.0)).xyz;

  gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix *
                vec4(aVertexPosition, 1.0);

  vPositionFromLight = uLightMVP * vec4(aVertexPosition, 1.0);
  vec3 kd = texture2D(uSampler, aTextureCoord).rgb;
  float sh_r = (dot(uPrecomputeLR[0], aPrecomputeLT[0]) + dot(uPrecomputeLR[1], aPrecomputeLT[1]) + dot(uPrecomputeLR[2], aPrecomputeLT[2])) / PI;
  float sh_g = (dot(uPrecomputeLG[0], aPrecomputeLT[0]) + dot(uPrecomputeLG[1], aPrecomputeLT[1]) + dot(uPrecomputeLG[2], aPrecomputeLT[2])) / PI;
  float sh_b = (dot(uPrecomputeLB[0], aPrecomputeLT[0]) + dot(uPrecomputeLB[1], aPrecomputeLT[1]) + dot(uPrecomputeLB[2], aPrecomputeLT[2])) / PI;
  vColor = vec3(kd.r * sh_r, kd.g * sh_g, kd.b * sh_b);
  //vColor = kd;
  //vColor = aPrecomputeLT[0];
}