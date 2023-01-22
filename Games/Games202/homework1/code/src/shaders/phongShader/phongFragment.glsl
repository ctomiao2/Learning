
precision highp float;

// Phong related variables
uniform sampler2D uSampler;
uniform vec3 uKd;
uniform vec3 uKs;
uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform vec3 uLightIntensity;
uniform vec2 uViewSize;

varying highp vec2 vTextureCoord;
varying highp vec3 vFragPos;
varying highp vec3 vNormal;

// Shadow map related variables
#define NUM_SAMPLES 40
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define NUM_RINGS 10
#define LIGHT_WIDTH 10.0
#define STRIDE 40.0
#define EPS 1e-3
#define PI 3.141592653589793
#define PI2 6.283185307179586

uniform sampler2D uShadowMap;
uniform sampler2D uShadowMap2;
uniform sampler2D uIndexMap;
uniform float uShadowMapPrecision;
//uniform mat4 uLightMVP;
uniform bool uReceiveShadow;

varying highp vec4 vPositionFromLight;
varying highp vec3 vVertexPos;

highp float rand_1to1(highp float x ) { 
  // -1 -1
  return fract(sin(x)*10000.0);
}

highp float rand_2to1(vec2 uv ) { 
  // 0 - 1
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

float unpackSAT(vec4 rgbaDepth) {
  return rgbaDepth.x*256.0*256.0*256.0 + rgbaDepth.y*256.0*256.0 + rgbaDepth.z*256.0 + rgbaDepth.w;
}

vec2 unpackIndex(vec4 rgbaDepth) {
  return vec2(256.0*(rgbaDepth.r*256.0 + rgbaDepth.g), 256.0*(rgbaDepth.b*256.0 + rgbaDepth.a));
}

float unpack(vec4 rgbaDepth) {
    const vec4 bitShift = vec4(1.0, 1.0/256.0, 1.0/(256.0*256.0), 1.0/(256.0*256.0*256.0));
    return dot(rgbaDepth, bitShift);
}

vec2 poissonCoords[65];

void poissonDiskSamples( const in vec2 randomSeed ) {
  float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
  float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

  float angle = rand_2to1( randomSeed ) * PI2;
  float radius = INV_NUM_SAMPLES;
  float radiusStep = radius;

  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    poissonCoords[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
    radius += radiusStep;
    angle += ANGLE_STEP;
  }
  
}

void uniformDiskSamples( const in vec2 randomSeed ) {

  float randNum = rand_2to1(randomSeed);
  float sampleX = rand_1to1( randNum ) ;
  float sampleY = rand_1to1( sampleX ) ;

  float angle = sampleX * PI2;
  float radius = sqrt(sampleY);

  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    poissonCoords[i] = vec2( radius * cos(angle) , radius * sin(angle)  );

    sampleX = rand_1to1( sampleY ) ;
    sampleY = rand_1to1( sampleX ) ;

    angle = sampleX * PI2;
    radius = sqrt(sampleY);
  }
}

float getBias() {
  vec3 lightDir = normalize(uLightPos - vFragPos);
  vec3 normal = normalize(vNormal);
  float A = 5.0/(2048.0*2.0);
  float B = 1.0 - dot(normal, lightDir);
  return max(A, 1.4*A*B);
}

float findBlocker( sampler2D shadowMap, float u, float v, float zReceiver) {
  float delta = STRIDE/2048.0;
  float ret = 0.0;
  float weight = 0.0;
  float bias = getBias();
  
  for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {
      int mat_idx = i / 16;
      vec2 offset = poissonCoords[i];
      vec2 test_coord = vec2(clamp(u + offset.x * delta, 0.0, 1.0), clamp(v + offset.y * delta, 0.0, 1.0));
      float test_depth = unpack(texture2D(shadowMap, test_coord).rgba);
      
      if (test_depth <= 0.0) {
        test_depth = 1.0;
      }
      

      if (zReceiver >= test_depth + bias) {
        ret += test_depth;
        weight += 1.0;
      }
  }
  if (weight <= 0.0)
    return 1.0;
  else
    return ret/weight;
}

float PCF(sampler2D shadowMap, vec4 shadowCoord, int sample_n) {
  float delta = STRIDE/2048.0;
  float ret = 0.0;
  float weight = min(float(sample_n), float(NUM_SAMPLES));
  float u = clamp(0.5 + 0.5 * shadowCoord.x/shadowCoord.w, 0.0, 1.0);
  float v = clamp(0.5 + 0.5 * shadowCoord.y/shadowCoord.w, 0.0, 1.0);
  float depth = clamp(0.5 + 0.5 * shadowCoord.z/shadowCoord.w, 0.0, 1.0);
  float bias = getBias();
  
  if (u <= 0.0 || u >= 1.0 || v <= 0.0 || v >= 1.0)
    return 1.0;
  
  for (int i = 0; i < NUM_SAMPLES; ++i) {
      vec2 offset = poissonCoords[i];
      vec2 test_coord = vec2(clamp(u + offset.x * delta, delta, 1.0 - delta), clamp(v + offset.y * delta, delta, 1.0 - delta));
      float test_depth = unpack(texture2D(shadowMap, test_coord).rgba);
      if (test_depth <= 0.0)
        test_depth = 1.0;
      if (depth < test_depth + bias) ret += 1.0;
      if (i >= sample_n - 1)
        break;
  }
  

  return ret/weight;
}

float PCSS(sampler2D shadowMap, vec4 coords){
  //if (!uReceiveShadow)
  //  return 1.0;
  float x = clamp(0.5 + 0.5 * coords.x/coords.w, 0.0, 1.0);
  float y = clamp(0.5 + 0.5 * coords.y/coords.w, 0.0, 1.0);
  float zReceiver = clamp(0.5 + 0.5 * coords.z/coords.w, 0.0, 1.0);
  // STEP 1: avgblocker depth
  float block_depth = findBlocker(shadowMap, x, y, zReceiver);
  // STEP 2: penumbra size
  float penumbra_size = 1.0;
  if (zReceiver > block_depth)
    penumbra_size = max(1.0, LIGHT_WIDTH * (zReceiver - block_depth)/block_depth);
  // STEP 3: filtering
  return PCF(shadowMap, coords, int(penumbra_size));
}

vec2 VSSM_Sample(sampler2D shadowMap, float u, float v, int sample_width) {
  int n1 = sample_width/2 + 1;
  int n2 = sample_width - n1;
  // SAT实际上存的是(0, 1)到(u, v)的矩形像素深度和
  float rx = clamp(u + uShadowMapPrecision * float(n2), 0.0, 1.0);
  float ry = clamp(v - uShadowMapPrecision * float(n2), 0.0, 1.0);
  float lx = clamp(u - uShadowMapPrecision * float(n1), 0.0, 1.0);
  float ly = clamp(v + uShadowMapPrecision * float(n1), 0.0, 1.0);
  float top_left = unpackSAT(texture2D(shadowMap, vec2(lx, ly)).rgba);
  float left = unpackSAT(texture2D(shadowMap, vec2(lx, ry)).rgba);
  float top = unpackSAT(texture2D(shadowMap, vec2(rx, ly)).rgba);
  float bottom_right = unpackSAT(texture2D(shadowMap, vec2(rx, ry)).rgba);
  float top_left_seq = unpackSAT(texture2D(uShadowMap2, vec2(lx, ly)).rgba);
  float left_seq = unpackSAT(texture2D(uShadowMap2, vec2(lx, ry)).rgba);
  float top_seq = unpackSAT(texture2D(uShadowMap2, vec2(rx, ly)).rgba);
  float bottom_right_seq = unpackSAT(texture2D(uShadowMap2, vec2(rx, ry)).rgba);

  vec2 top_left_rc = unpackIndex(texture2D(uIndexMap, vec2(lx, ly)).rgba);
  vec2 left_rc = unpackIndex(texture2D(uIndexMap, vec2(lx, ry)).rgba);
  vec2 top_rc = unpackIndex(texture2D(uIndexMap, vec2(rx, ly)).rgba);
  vec2 bottom_right_rc = unpackIndex(texture2D(uIndexMap, vec2(rx, ry)).rgba);
  int top_left_n = int(top_left_rc.x) * int(top_left_rc.y);
  int left_n = int(left_rc.x) * int(left_rc.y);
  int top_n = int(top_rc.x) * int(top_rc.y);
  int bottom_right_n = int(bottom_right_rc.x) * int(bottom_right_rc.y);
  int total_n = bottom_right_n - (left_n  + top_n - top_left_n);

  int total = sample_width*sample_width;
  float sum_depth = bottom_right - (left  + top - top_left);
  float sum_seq_depth = (bottom_right_seq - (left_seq  + top_seq - top_left_seq))/100.0;
  float mean_depth = sum_depth / float(total_n);
  float mean_seq_depth = sum_seq_depth / float(total_n);
  
  //return vec2(float(total - total_n), 0.0);
  return vec2(mean_depth, mean_seq_depth);
}

float VSSM(sampler2D shadowMap, vec4 coords) {
  float x = clamp(0.5 + 0.5 * coords.x/coords.w, 0.0, 1.0);
  float y = clamp(0.5 + 0.5 * coords.y/coords.w, 0.0, 1.0);
  float zReceiver = clamp(0.5 + 0.5 * coords.z/coords.w, 0.0, 1.0);
  vec2 sat_depth = VSSM_Sample(shadowMap, x, y, 6);
  float mean_depth = sat_depth.x;
  float mean_seq_depth = sat_depth.y;

  //return mean_depth;

  float var_depth = max(0.0, mean_seq_depth - mean_depth * mean_depth);
  
  float prob_unocc = 1.0;
  float penumbra_size = 1.0;

  if (var_depth > 1e-6) {
    // 切比雪夫不等式
    prob_unocc = var_depth / (var_depth + (zReceiver - mean_depth) * (zReceiver - mean_depth));
    // z_unocc * prob_unocc + z_occ * (1-prob_unocc) = mean_depth
    float block_depth = (mean_depth - prob_unocc * zReceiver) / (1.0 - prob_unocc);
    if (block_depth > 0.0 && zReceiver > block_depth)
      penumbra_size = min(20.0, max(1.0, LIGHT_WIDTH * (zReceiver - block_depth)/block_depth));
  }

  if (penumbra_size > 1.0) {
    sat_depth = VSSM_Sample(shadowMap, x, y, int(penumbra_size));
    mean_depth = sat_depth.x;
    mean_seq_depth = sat_depth.y;
    var_depth = max(0.0, mean_seq_depth - mean_depth * mean_depth);
    if (var_depth > 1e-6)
      return var_depth / (var_depth + (zReceiver - mean_depth) * (zReceiver - mean_depth));
  }
  else return 1.0;
}

float useShadowMap(sampler2D shadowMap, vec4 shadowCoord){
  //if (!uReceiveShadow)
  //  return 1.0;
  float x = clamp(0.5 + 0.5 * shadowCoord.x/shadowCoord.w, 0.0, 1.0);
  float y = clamp(0.5 + 0.5 * shadowCoord.y/shadowCoord.w, 0.0, 1.0);
  if (x <= 0.0 || x >= 1.0 || y <= 0.0 || y >= 1.0)
    return 1.0;
  float minDepth = unpack(texture2D(shadowMap, vec2(x, y)).rgba);
  float depth = clamp(0.5 + 0.5 * shadowCoord.z/shadowCoord.w, 0.0, 1.0);
  float bias = getBias();
  if (depth < minDepth + bias) return 1.0;
  else return 0.0;
}

vec3 blinnPhong() {
  vec3 color = texture2D(uSampler, vTextureCoord).rgb;
  color = pow(color, vec3(2.2));

  vec3 ambient = 0.05 * color;

  vec3 lightDir = normalize(uLightPos - vFragPos);
  vec3 normal = normalize(vNormal);
  float diff = max(dot(lightDir, normal), 0.0);
  vec3 light_atten_coff =
      uLightIntensity / pow(length(uLightPos - vFragPos), 2.0);
  vec3 diffuse = diff * light_atten_coff * color;

  vec3 viewDir = normalize(uCameraPos - vFragPos);
  vec3 halfDir = normalize((lightDir + viewDir));
  float spec = pow(max(dot(halfDir, normal), 0.0), 32.0);
  vec3 specular = uKs * light_atten_coff * spec;

  vec3 radiance = (ambient + diffuse + specular);
  vec3 phongColor = pow(radiance, vec3(1.0 / 2.2));
  return phongColor;
}

void main(void) {
  /*
  float x = 0.5 + 0.5 * vPositionFromLight.x/vPositionFromLight.w;//clamp(0.5 + 0.5 * vPositionFromLight.x/vPositionFromLight.w, 0.0, 1.0);
  float y = 0.5 + 0.5 * vPositionFromLight.y/vPositionFromLight.w;//clamp(0.5 + 0.5 * vPositionFromLight.y/vPositionFromLight.w, 0.0, 1.0);
  float curDepth = clamp(0.5 + 0.5 * vPositionFromLight.z/vPositionFromLight.w, 0.0, 1.0);
  float depth = unpack(texture2D(uShadowMap, vec2(x, y)).rgba);
  
  if (depth > curDepth)
    gl_FragColor = vec4(100.0*(depth - curDepth), 0, 0, 1.0);
  else
    gl_FragColor = vec4(0, 100.0*(curDepth - depth), 0, 1.0);
  */
  
  float visibility;

  // poissonDiskSamples(vec2(vPositionFromLight.x, vPositionFromLight.y));
  // initCoords();

  // visibility = useShadowMap(uShadowMap, vPositionFromLight);
  // visibility = PCF(uShadowMap, vPositionFromLight, 9);
  // visibility = PCSS(uShadowMap, vPositionFromLight);
  visibility = VSSM(uShadowMap, vPositionFromLight);
  vec3 phongColor = blinnPhong();
  
  // gl_FragColor = vec4(visibility, 0, 0, 1.0);
  gl_FragColor = vec4(phongColor * visibility, 1.0);
  return;
  float r = 1.0;
  if (visibility > 0.0)
    gl_FragColor = vec4(visibility*r, 0, 0, 1.0);
  else
    gl_FragColor = vec4(0, -visibility*r, 0, 1.0);
  //gl_FragColor = vec4(phongColor, 1.0);
}