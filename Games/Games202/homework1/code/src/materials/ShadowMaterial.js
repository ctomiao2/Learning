class ShadowMaterial extends Material {

    constructor(light, translate, scale, vertexShader, fragmentShader, extra_data, test_flag) {
        let lightMVP = light.CalcLightMVP(translate, scale);
        let uniforms = { 'uLightMVP': { type: 'matrix4fv', value: lightMVP } };
        if (extra_data) {
            if (extra_data['uDepthBias'] != undefined)
                uniforms['uDepthBias'] = { type: '1f', value: extra_data['uDepthBias'] };
        }

        const canvas = document.querySelector('#glcanvas');
        uniforms['uViewSize'] = { type: '2fv', value: [window.screen.width, window.screen.height] };

        if (test_flag) {
            uniforms['uTestFlag'] = { type: '1f', value: 1.0 };
            uniforms['uShadowMap'] = { type: 'texture', value: light.fbo };
            super(uniforms, [], vertexShader, fragmentShader);
        }
        else {
            uniforms['uTestFlag'] = { type: '1f', value: 0.0 };
            super(uniforms, [], vertexShader, fragmentShader, light.fbo);
        }
        this.light = light;
        this.light.Register(this);
        this.translate = translate;
        this.scale = scale;
        this.extra_data = extra_data;
    }

    updateLight() {
        this.uniforms['uLightMVP'].value = this.light.CalcLightMVP(this.translate, this.scale);
    }
}

async function buildShadowMaterial(light, translate, scale, vertexPath, fragmentPath, extra_data, test_flag) {


    let vertexShader = await getShaderString(vertexPath);
    let fragmentShader = await getShaderString(fragmentPath);

    return new ShadowMaterial(light, translate, scale, vertexShader, fragmentShader, extra_data, test_flag);

}