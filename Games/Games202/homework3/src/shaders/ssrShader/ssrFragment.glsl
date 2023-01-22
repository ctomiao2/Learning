
#ifdef GL_ES
precision highp float;
#endif

uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform vec3 uLightRadiance;
uniform sampler2D uGDiffuse;
uniform sampler2D uGDepth;
uniform sampler2D uGNormalWorld;
uniform sampler2D uGShadow;
uniform sampler2D uGPosWorld;

varying mat4 vWorldToScreen;
varying highp vec4 vPosWorld;

#define M_PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307
#define INV_PI 0.31830988618
#define INV_TWO_PI 0.15915494309

float Rand1(inout float p) {
  p = fract(p * .1031);
  p *= p + 33.33;
  p *= p + p;
  return fract(p);
}

vec2 Rand2(inout float p) {
  return vec2(Rand1(p), Rand1(p));
}

float InitRand(vec2 uv) {
	vec3 p3  = fract(vec3(uv.xyx) * .1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

vec3 SampleHemisphereUniform(inout float s, out float pdf) {
  vec2 uv = Rand2(s);
  float z = uv.x;
  float phi = uv.y * TWO_PI;
  float sinTheta = sqrt(1.0 - z*z);
  vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
  pdf = INV_TWO_PI;
  return dir;
}

vec3 SampleHemisphereCos(inout float s, out float pdf) {
  vec2 uv = Rand2(s);
  float z = sqrt(1.0 - uv.x);
  float phi = uv.y * TWO_PI;
  float sinTheta = sqrt(uv.x);
  vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
  pdf = z * INV_PI;
  return dir;
}

void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
  float sign_ = sign(n.z);
  if (n.z == 0.0) {
    sign_ = 1.0;
  }
  float a = -1.0 / (sign_ + n.z);
  float b = n.x * n.y * a;
  b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
  b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

vec4 Project(vec4 a) {
  return a / a.w;
}

float GetDepth(vec3 posWorld) {
  float depth = (vWorldToScreen * vec4(posWorld, 1.0)).w;
  return depth;
}

/*
 * Transform point from world space to screen space([0, 1] x [0, 1])
 *
 */
vec2 GetScreenCoordinate(vec3 posWorld) {
  vec2 uv = Project(vWorldToScreen * vec4(posWorld, 1.0)).xy * 0.5 + 0.5;
  return uv;
}

float GetGBufferDepth(vec2 uv) {
  float depth = texture2D(uGDepth, uv).x;
  if (depth < 1e-2) {
    depth = 1000.0;
  }
  return depth;
}

vec3 GetGBufferNormalWorld(vec2 uv) {
  vec3 normal = texture2D(uGNormalWorld, uv).xyz;
  return normal;
}

vec3 GetGBufferPosWorld(vec2 uv) {
  vec3 posWorld = texture2D(uGPosWorld, uv).xyz;
  return posWorld;
}

float GetGBufferuShadow(vec2 uv) {
  float visibility = texture2D(uGShadow, uv).x;
  return visibility;
}

vec3 GetGBufferDiffuse(vec2 uv) {
  vec3 diffuse = texture2D(uGDiffuse, uv).xyz;
  diffuse = pow(diffuse, vec3(2.2));
  return diffuse;
}

/*
 * Evaluate diffuse bsdf value.
 *
 * wi, wo are all in world space.
 * uv is in screen space, [0, 1] x [0, 1].
 *
 */

vec3 EvalDiffuse(vec3 wi, vec3 wo, vec2 uv) {
  vec3 L = vec3(0.0);
  // 菲涅尔项
  float F0 = 1.;
  vec3 h = normalize(wi + wo);
  vec3 v = wi;
  float Fresnel_term = F0 + (1.0 - F0) * pow((1.0 - dot(h,v)),5.);
  // 法线分布项
  float alpha = F0;
  vec3 n = GetGBufferNormalWorld(uv);
  float NDF_div = pow(pow(dot(n,h),2.) * (pow(alpha,2.) - 1.) + 1., 2.);
  float NDF_GGXTR = pow(alpha,2.) / (M_PI * NDF_div);
  // 几何遮蔽项
  alpha = pow(( alpha + 1.0 ) * 0.5, 2.);
  float k = alpha * 0.5;
  float G1L = dot(n,wo) / ( dot(n,wo) * (1.0 - k) + k);
  float G1V = dot(n,wi) / ( dot(n,wi) * (1.0 - k) + k);
  float Graphic_term = G1L * G1V;
  // BRDF项
  vec3 BRDF = GetGBufferDiffuse(uv) * Fresnel_term * Graphic_term * NDF_GGXTR / (4.0 * dot(n,wi) * dot(n,wo));
  return BRDF;
}

/*
 * Evaluate directional light with shadow map
 * uv is in screen space, [0, 1] x [0, 1].
 *
 */
vec3 EvalDirectionalLight(vec2 uv) {
  vec3 posW = GetGBufferPosWorld(uv);
  vec3 wi = normalize(uLightDir);
  vec3 wo = normalize(uCameraPos - posW);
  vec3 brdf = EvalDiffuse(wi, wo, uv);
  return uLightRadiance * brdf * GetGBufferuShadow(uv);
}

#define STEP 0.8
#define MAX_STEPS 20
#define EPS 1e-2

bool outScreen(vec3 pos){
  vec2 uv = GetScreenCoordinate(pos);
  return any(bvec4(lessThan(uv, vec2(0.0)), greaterThan(uv, vec2(1.0))));
}
bool atFront(vec3 pos){
  return GetDepth(pos) < GetGBufferDepth(GetScreenCoordinate(pos));
}

bool RayMarch(vec3 ori, vec3 dir, out vec3 hitPos) {
  bool intersect = false;
  float st = STEP;
  vec3 current = ori;
  for (int i = 0; i < MAX_STEPS; i++){
    if(outScreen(current)){
      break;
    }
    if(atFront(current + dir * st)){
      current += dir * st;
    } else {
      if(st <= EPS){
        intersect = true;
        hitPos = current + dir * st;
        break;
      } else {
        st *= 0.5;
      }
    }
  }
  return intersect;
}

#define SAMPLE_NUM 10
vec3 EvalIndirectLight(vec3 pos){
  float pdf, seed = dot(pos, vec3(100.0));
  vec3 Li = vec3(0.0), dir, hitPos;
  vec2 uv = GetScreenCoordinate(pos);
  vec3 normal = GetGBufferNormalWorld(uv);
  vec3 b1, b2;
  LocalBasis(normal, b1, b2);
  vec3 pos2Camera = normalize(uCameraPos - pos);
  mat3 TBN = mat3(b1, b2, normal);
  for(int i = 0; i < SAMPLE_NUM;i++){
    dir = normalize(TBN * SampleHemisphereCos(seed, pdf));
    if(dot(pos2Camera, normal) > 0.0 && RayMarch(pos, dir, hitPos)){
      vec3 L = EvalDiffuse(dir, pos2Camera, uv) / pdf;
      vec3 wo = normalize(uCameraPos - hitPos);
      vec3 wi = normalize(uLightDir);
      vec2 hitUV = GetScreenCoordinate(hitPos);
      L *= EvalDirectionalLight(hitUV);
      Li += L;
    }
  }
  return Li / float(SAMPLE_NUM);
}

void main() {
  float s = InitRand(gl_FragCoord.xy);

  vec3 L = vec3(0.0), Li;
  L = EvalDirectionalLight(GetScreenCoordinate(vPosWorld.xyz));
  Li = EvalIndirectLight(vPosWorld.xyz);
  L += Li;
  vec3 color = pow(clamp(L, vec3(0.0), vec3(1.0)), vec3(1.0 / 2.2));

  gl_FragColor = vec4(color, 1.0);
}

