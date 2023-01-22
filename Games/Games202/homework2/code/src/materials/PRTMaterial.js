class PRTMaterial extends Material {

    constructor(color, specular, light, translate, scale, vertexShader, fragmentShader) {
        // let lightMVP = light.CalcLightMVP(translate, scale);
        let lightIntensity = light.mat.GetIntensity();

        super({
            // Phong
            'uSampler': { type: 'texture', value: color },
            'uKs': { type: '3fv', value: specular },
            'uLightRadiance': { type: '3fv', value: lightIntensity },
            // Shadow
            'uShadowMap': { type: 'texture', value: light.fbo },
            //'uLightMVP': { type: 'matrix4fv', value: lightMVP },
            'uPrecomputeLR' : { type: 'matrix3fv', value: null },
            'uPrecomputeLG' : { type: 'matrix3fv', value: null },
            'uPrecomputeLB' : { type: 'matrix3fv', value: null },

        }, ['aPrecomputeLT'], vertexShader, fragmentShader, null);
    }

    updateUniforms() {
        var uPrecomputeLR = mat3.create();
        var uPrecomputeLG = mat3.create();
        var uPrecomputeLB = mat3.create();
        var data = precomputeL[guiParams.envmapId];
        for (var i = 0; i < 9; ++i) {
            uPrecomputeLR[i] = data[i][0];
            uPrecomputeLG[i] = data[i][1];
            uPrecomputeLB[i] = data[i][2];
        }
        this.uniforms['uPrecomputeLR'].value = uPrecomputeLR;
        this.uniforms['uPrecomputeLG'].value = uPrecomputeLG;
        this.uniforms['uPrecomputeLB'].value = uPrecomputeLB;
    }
}

async function buildPRTMaterial(color, specular, light, translate, scale, vertexPath, fragmentPath) {


    let vertexShader = await getShaderString(vertexPath);
    let fragmentShader = await getShaderString(fragmentPath);

    return new PRTMaterial(color, specular, light, translate, scale, vertexShader, fragmentShader);

}