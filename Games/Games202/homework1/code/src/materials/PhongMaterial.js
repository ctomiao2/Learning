class PhongMaterial extends Material {

    constructor(color, specular, light, translate, scale, vertexShader, fragmentShader, extra_data) { 
        let lightMVP = light.CalcLightMVP(translate, scale);
        let lightIntensity = light.mat.GetIntensity();
        const canvas = document.querySelector('#glcanvas');

        super({
            // Phong
            'uSampler': { type: 'texture', value: color },
            'uKs': { type: '3fv', value: specular },
            'uLightIntensity': { type: '3fv', value: lightIntensity },
            // Shadow
            'uShadowMap': { type: 'texture', value: light.fbo },
            'uShadowMap2': { type: 'texture2d', value: null },
            'uIndexMap': { type: 'texture2d', value: null },
            'uLightMVP': { type: 'matrix4fv', value: lightMVP },
            'uShadowMapPrecision': { type: '1f', value: 1.0/resolution },
            'uViewSize': { type: '2fv', value: [window.screen.width, window.screen.height] },
            'uReceiveShadow': { type: '1i', value: extra_data['uReceiveShadow'] },
        }, [], vertexShader, fragmentShader);
        
        this.light = light;
        this.light.Register(this);
        this.translate = translate;
        this.scale = scale;
        this.extra_data = extra_data;
    }

    updateLight() {
        this.uniforms['uLightMVP'].value = this.light.CalcLightMVP(this.translate, this.scale);
        //this.uniforms['uShadowMap'].value = this.light.fbo;
    }
}

async function buildPhongMaterial(color, specular, light, translate, scale, vertexPath, fragmentPath, extra_data) {


    let vertexShader = await getShaderString(vertexPath);
    let fragmentShader = await getShaderString(fragmentPath);

    return new PhongMaterial(color, specular, light, translate, scale, vertexShader, fragmentShader, extra_data);

}