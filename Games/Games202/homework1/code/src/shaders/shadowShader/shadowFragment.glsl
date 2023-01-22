precision highp float;
precision highp sampler2D;

uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform float uDepthBias;

uniform vec2 uViewSize;
uniform sampler2D uShadowMap;
uniform float uTestFlag;

varying highp vec3 vNormal;
varying highp vec2 vTextureCoord;
varying highp vec4 vOutPosition;

vec4 pack (float depth) {
    // 使用rgba 4字节共32位来存储z值,1个字节精度为1/256
    const vec4 bitShift = vec4(1.0, 256.0, 256.0 * 256.0, 256.0 * 256.0 * 256.0);
    const vec4 bitMask = vec4(1.0/256.0, 1.0/256.0, 1.0/256.0, 0.0);
    // gl_FragCoord:片元的坐标,fract():返回数值的小数部分
    vec4 rgbaDepth = fract(depth * bitShift); //计算每个点的z值
    rgbaDepth -= rgbaDepth.gbaa * bitMask; // Cut off the value which do not fit in 8 bits
    return rgbaDepth;
}

float unpack(vec4 rgbaDepth) {
    const vec4 bitShift = vec4(1.0, 1.0/256.0, 1.0/(256.0*256.0), 1.0/(256.0*256.0*256.0));
    return dot(rgbaDepth, bitShift);
}

void main(){
  //float z = vOutPosition.z/vOutPosition.w;
  // gl_FragColor = vec4(5000.0*(abs(z2-z)), 0.0, 0.0, 1.0);
  if (uTestFlag > 0.0) {
    float x = clamp(0.5 + 0.5 * vOutPosition.x/vOutPosition.w, 0.0, 1.0);
    float y = clamp(0.5 + 0.5 * vOutPosition.y/vOutPosition.w, 0.0, 1.0);
    float fx = clamp(gl_FragCoord.x / uViewSize.x, 0.0, 1.0);
    float fy = clamp(gl_FragCoord.y / (uViewSize.y), 0.0, 1.0);
    float fz = gl_FragCoord.z;
    float curDepth = fz;//clamp(0.5 + 0.5 * vOutPosition.z/vOutPosition.w, 0.0, 1.0);
    float depth = unpack(texture2D(uShadowMap, vec2(fx, fy)).rgba);

    if (depth > curDepth)
      gl_FragColor = vec4(100.0*(depth - curDepth), 0, 0, 1.0);
    else
      gl_FragColor = vec4(0, 100.0*(curDepth - depth), 0, 1.0);
    
    //gl_FragColor = vec4(curDepth, 0, 0, 1.0);
    /*
    if (y > fy)
      gl_FragColor = vec4(1000.0*(y - fy), 0, 0, 1.0);
    else
      gl_FragColor = vec4(0, 1000.0*(fy - y), 0, 1.0);
    
    if (x > fx)
      gl_FragColor = vec4(1000.0*(x - fx), 0, 0, 1.0);
    else
      gl_FragColor = vec4(0, 1000.0*(fx - x), 0, 1.0);
    */
    gl_FragColor = vec4(depth*30.0, 0, 0, 1.0);
  } else {
    gl_FragColor = pack(gl_FragCoord.z); // vec4(gl_FragCoord.z, 0, 0, 1.0);
  }
}