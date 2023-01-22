class DirectionalLight {

    constructor(lightIntensity, lightColor, lightPos, focalPoint, lightUp, hasShadowMap, gl) {
        this.mesh = Mesh.cube(setTransform(0, 0, 0, 0.2, 0.2, 0.2, 0));
        this.mat = new EmissiveMaterial(lightIntensity, lightColor);
        this.originPos = [lightPos[0], lightPos[1], lightPos[2]];
        this.lightPos = lightPos;
        this.focalPoint = focalPoint;
        this.lightUp = lightUp
        this.rot = 0.0;
        this.hasShadowMap = hasShadowMap;
        var fbo_objects = new FBO(gl);
        this.fbo = fbo_objects[0];
        this.depthBuffer = fbo_objects[2];

        if (!this.fbo) {
            console.log("无法设置帧缓冲区对象");
            return;
        }
        this.cast_materials = [];
    }

    updateLightPos() {
        var cos_theta = Math.cos(this.rot);
        var sin_theta = Math.sin(this.rot);
        var x = cos_theta * this.originPos[0] - sin_theta * this.originPos[2];
        var z = sin_theta * this.originPos[0] + cos_theta * this.originPos[2];
        this.lightPos[0] = x;
        this.lightPos[2] = z;
        // update fbo
        const canvas = document.querySelector('#glcanvas');
        const gl = canvas.getContext('webgl');
        gl.bindTexture(gl.TEXTURE_2D, this.fbo.texture);
        gl.bindRenderbuffer(gl.RENDERBUFFER, this.depthBuffer);
        gl.bindFramebuffer(gl.FRAMEBUFFER, this.fbo);
        gl.clear(gl.DEPTH_BUFFER_BIT | gl.COLOR_BUFFER_BIT);
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        gl.bindTexture(gl.TEXTURE_2D, null);
        gl.bindRenderbuffer(gl.RENDERBUFFER, null);
        
        for (let i = 0; i < this.cast_materials.length; i++) {
            //this.cast_materials[i].frameBuffer = this.fbo;
            this.cast_materials[i].updateLight();
        }

        console.log('updateLightPos');
    }

    rotate(delta_angle) {
        this.rot += Math.PI * delta_angle/180.0;
        this.updateLightPos();
    }

    Register(material) {
        this.cast_materials.push(material);
    }

    CalcLightMVP(translate, scale) {
        let lightMVP = mat4.create();
        let modelMatrix = mat4.create();
        let viewMatrix = mat4.create();
        let projectionMatrix = mat4.create();

        // Model transform
        mat4.identity(modelMatrix);
		mat4.translate(modelMatrix, modelMatrix, translate);
        mat4.scale(modelMatrix, modelMatrix, scale);
        // View transform
        mat4.lookAt(viewMatrix, this.lightPos, this.focalPoint, this.lightUp);
        // Projection transform
        var r = 100, l = -r, t = 100, b = -t, n = 1e-2, f = 10000;
        mat4.ortho(projectionMatrix, l, r, b, t, n, f);
        mat4.multiply(lightMVP, projectionMatrix, viewMatrix);
        mat4.multiply(lightMVP, lightMVP, modelMatrix);

        return lightMVP;
    }
}
