class WebGLRenderer {
    meshes = [];
    shadowMeshes = [];
    shadowMeshes2 = [];
    lights = [];

    constructor(gl, camera) {
        this.gl = gl;
        this.camera = camera;
        this.enable_vssm = true;
        this.use_index_map = true;
        this.texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.texture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, resolution, resolution, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
        gl.bindTexture(gl.TEXTURE_2D, null);
        this.index_texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.index_texture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, resolution, resolution, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
        gl.bindTexture(gl.TEXTURE_2D, null);
    }

    addLight(light) {
        this.lights.push({
            entity: light,
            meshRender: new MeshRender(this.gl, light.mesh, light.mat)
        });
    }
    addMeshRender(mesh) {
        this.meshes.push(mesh);
    }
    addShadowMeshRender(mesh) { this.shadowMeshes.push(mesh); }

    addShadowMeshRender2(mesh) { this.shadowMeshes2.push(mesh); }

    getDepthRGBA(depth) {
        var m1 = 256;
        var m2 = m1*256;
        var m3 = m2*256;
        var r = Math.floor(depth/m3);
        var g = Math.floor((depth - m3*r)/m2);
        var b = Math.floor((depth - m3*r - m2*g)/m1);
        var a = parseInt(depth) % m1;
        return [r, g, b, a];
    }

    getIndexRGBA(row, col) {
        return [Math.floor(row/256), row % 256, Math.floor(col/256), col % 256];
    }

    generateSAT(packDepthBuff, packDepthSeqBuff, packIndexBuff) {
        // 倒转y方向
        for (var i = 0; i < resolution/2; ++i) {
            for (var j = 0; j < resolution * 4; ++j) {
                var idx1 = i * resolution * 4 + j;
                var idx2 = (resolution - 1 - i) * resolution * 4 + j;
                var tmp = packDepthBuff[idx1];
                packDepthBuff[idx1] = packDepthBuff[idx2];
                packDepthBuff[idx2] = tmp;
            }
        }
        
        var b1 = 1.0/256.0;
        var b2 = b1/256.0;
        var b3 = b2/256.0;
        var last_row_sum = [];
        var last_row_seq_sum = [];
       
        // 计算第一行
        for (var idx = 0; idx < resolution; ++idx) {
            var i = 4 * idx;
            var sum_depth = packDepthBuff[i] + packDepthBuff[i+1]*b1 + packDepthBuff[i+2]*b2 + packDepthBuff[i+3]*b3; // 256*depth
            var sum_depth_seq = 100 * sum_depth * sum_depth / 256; // 100*256*depth^2
            if (idx > 0) {
                sum_depth += last_row_sum[idx-1];
                sum_depth_seq += last_row_seq_sum[idx-1];
            }
            last_row_sum.push(sum_depth);
            last_row_seq_sum.push(sum_depth_seq);
            
            var sumDepthRGBA = this.getDepthRGBA(sum_depth);
            packDepthBuff[i] = sumDepthRGBA[0];
            packDepthBuff[i+1] = sumDepthRGBA[1];
            packDepthBuff[i+2] = sumDepthRGBA[2];
            packDepthBuff[i+3] = sumDepthRGBA[3];
            var sumDepthSeqRGBA = this.getDepthRGBA(sum_depth_seq);
            packDepthSeqBuff[i] = sumDepthSeqRGBA[0];
            packDepthSeqBuff[i+1] = sumDepthSeqRGBA[1];
            packDepthSeqBuff[i+2] = sumDepthSeqRGBA[2];
            packDepthSeqBuff[i+3] = sumDepthSeqRGBA[3];
            if (packIndexBuff) {
                var indexRGBA = this.getIndexRGBA(0, idx);
                packIndexBuff[i] = indexRGBA[0];
                packIndexBuff[i+1] = indexRGBA[1];
                packIndexBuff[i+2] = indexRGBA[2];
                packIndexBuff[i+3] = indexRGBA[3];
            }
        }

        var first_col_sum = [last_row_sum[0]];
        var first_col_seq_sum = [last_row_seq_sum[0]];
        // 计算第一列
        for (var idx = 1; idx < resolution; ++idx) {
            var i = idx * resolution * 4;
            var sum_depth = packDepthBuff[i] + packDepthBuff[i+1]*b1 + packDepthBuff[i+2]*b2 + packDepthBuff[i+3]*b3;
            var sum_depth_seq = 100 * sum_depth * sum_depth / 256;
            sum_depth += first_col_sum[idx-1];
            sum_depth_seq += first_col_seq_sum[idx-1];
            first_col_sum.push(sum_depth);
            first_col_seq_sum.push(sum_depth_seq);
            // 不存总和直接存期望值, 否则精度不够
            //var depth = sum_depth/(idx+1);
            //var depth_seq = sum_depth_seq/(idx+1);

            var sumDepthRGBA = this.getDepthRGBA(sum_depth);
            packDepthBuff[i] = sumDepthRGBA[0];
            packDepthBuff[i+1] = sumDepthRGBA[1];
            packDepthBuff[i+2] = sumDepthRGBA[2];
            packDepthBuff[i+3] = sumDepthRGBA[3];
            var sumDepthSeqRGBA = this.getDepthRGBA(sum_depth_seq);
            packDepthSeqBuff[i] = sumDepthSeqRGBA[0];
            packDepthSeqBuff[i+1] = sumDepthSeqRGBA[1];
            packDepthSeqBuff[i+2] = sumDepthSeqRGBA[2];
            packDepthSeqBuff[i+3] = sumDepthSeqRGBA[3];
            if (packIndexBuff) {
                var indexRGBA = this.getIndexRGBA(idx, 0);
                packIndexBuff[i] = indexRGBA[0];
                packIndexBuff[i+1] = indexRGBA[1];
                packIndexBuff[i+2] = indexRGBA[2];
                packIndexBuff[i+3] = indexRGBA[3];
            }
        }

        // 再计算中间的行列
        for (var i = 1; i < resolution; ++i) {
            var cur_row_sum = [first_col_sum[i]];
            var cur_row_seq_sum = [first_col_seq_sum[i]];
            for (var j = 1; j < resolution; ++j) {
                let k = i * resolution * 4 + j * 4;
                let sum_depth = packDepthBuff[k] + packDepthBuff[k+1]*b1 + packDepthBuff[k+2]*b2 + packDepthBuff[k+3]*b3;
                let sum_depth_seq = 100 * sum_depth * sum_depth / 256;
                sum_depth += (cur_row_sum[j-1] + last_row_sum[j] - last_row_sum[j-1]);
                sum_depth_seq += (cur_row_seq_sum[j-1] + last_row_seq_sum[j] - last_row_seq_sum[j-1]);
                cur_row_sum.push(sum_depth);
                cur_row_seq_sum.push(sum_depth_seq);
                var sumDepthRGBA = this.getDepthRGBA(sum_depth);
                packDepthBuff[k] = sumDepthRGBA[0];
                packDepthBuff[k+1] = sumDepthRGBA[1];
                packDepthBuff[k+2] = sumDepthRGBA[2];
                packDepthBuff[k+3] = sumDepthRGBA[3];
                var sumDepthSeqRGBA = this.getDepthRGBA(sum_depth_seq);
                packDepthSeqBuff[k] = sumDepthSeqRGBA[0];
                packDepthSeqBuff[k+1] = sumDepthSeqRGBA[1];
                packDepthSeqBuff[k+2] = sumDepthSeqRGBA[2];
                packDepthSeqBuff[k+3] = sumDepthSeqRGBA[3];
                if (packIndexBuff) {
                    var indexRGBA = this.getIndexRGBA(i, j);
                    packIndexBuff[k] = indexRGBA[0];
                    packIndexBuff[k+1] = indexRGBA[1];
                    packIndexBuff[k+2] = indexRGBA[2];
                    packIndexBuff[k+3] = indexRGBA[3];
                }
            }
            last_row_sum = cur_row_sum;
            last_row_seq_sum = cur_row_seq_sum;
        }
    }

    render() {
        const gl = this.gl;

        for (let l = 0; l < this.lights.length; l++) {
            gl.bindFramebuffer(gl.FRAMEBUFFER, this.lights[l].entity.fbo);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
            gl.clearColor(0.0, 0.0, 0.0, 1.0); // Clear to black, fully opaque
            gl.clearDepth(1.0); // Clear everything
            gl.enable(gl.DEPTH_TEST); // Enable depth testing
            gl.depthFunc(gl.LEQUAL); // Near things obscure far things
        }

        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        gl.clearColor(0.0, 0.0, 0.0, 1.0); // Clear to black, fully opaque
        gl.clearDepth(1.0); // Clear everything
        gl.enable(gl.DEPTH_TEST); // Enable depth testing
        gl.depthFunc(gl.LEQUAL); // Near things obscure far things

        console.assert(this.lights.length != 0, "No light");
        console.assert(this.lights.length == 1, "Multiple lights");
        var ret = {};

        for (let l = 0; l < this.lights.length; l++) {
            // Draw light
            // TODO: Support all kinds of transform
            this.lights[l].meshRender.mesh.transform.translate = this.lights[l].entity.lightPos;
            this.lights[l].meshRender.draw(this.camera);

            // Shadow pass
            if (this.lights[l].entity.hasShadowMap == true) {
                for (let i = 0; i < this.shadowMeshes.length; i++) {
                    this.shadowMeshes[i].draw(this.camera);
                }
            }
            
            if (this.enable_vssm) {
                gl.bindFramebuffer(gl.FRAMEBUFFER, this.lights[l].entity.fbo);
                let SM_DepthBuf = new Uint8Array(resolution * resolution * 4);
                let SM_DepthSeqBuf = new Uint8Array(resolution * resolution * 4);
                let SM_IndexBuf = null;
                if (this.use_index_map)
                    SM_IndexBuf = new Uint8Array(resolution * resolution * 4);

                // VSSM SAT pass
                this.gl.readPixels(0, 0, resolution, resolution, gl.RGBA, gl.UNSIGNED_BYTE, SM_DepthBuf);
                this.generateSAT(SM_DepthBuf, SM_DepthSeqBuf, SM_IndexBuf);
                const canvas = document.querySelector('#glcanvas');
                //console.log(canvas.clientWidth + ',' + canvas.clientHeight);
                gl.bindTexture(gl.TEXTURE_2D, this.lights[l].entity.fbo.texture);
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, resolution, resolution, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
                // TODO, 再弄个texture存索引
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, resolution, resolution, 0, gl.RGBA, gl.UNSIGNED_BYTE, SM_DepthBuf);
                gl.bindFramebuffer(gl.FRAMEBUFFER, null);
                gl.bindTexture(gl.TEXTURE_2D, null);
                gl.bindTexture(gl.TEXTURE_2D, this.texture);
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, resolution, resolution, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, resolution, resolution, 0, gl.RGBA, gl.UNSIGNED_BYTE, SM_DepthSeqBuf);
                gl.bindTexture(gl.TEXTURE_2D, null);
                
                if (this.use_index_map) {
                    gl.bindTexture(gl.TEXTURE_2D, this.index_texture);
                    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, resolution, resolution, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
                    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
                    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
                    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, resolution, resolution, 0, gl.RGBA, gl.UNSIGNED_BYTE, SM_IndexBuf);
                    gl.bindTexture(gl.TEXTURE_2D, null);
                }
            }

            if (this.lights[l].entity.hasShadowMap == true) {
                for (let i = 0; i < this.shadowMeshes2.length; i++) {
                    this.shadowMeshes2[i].draw(this.camera);
                }
            }

            // Camera pass
            for (let i = 0; i < this.meshes.length; i++) {
                this.gl.useProgram(this.meshes[i].shader.program.glShaderProgram);
                this.gl.uniform3fv(this.meshes[i].shader.program.uniforms.uLightPos, this.lights[l].entity.lightPos);
                this.meshes[i].material.uniforms['uShadowMap2'].value = this.texture;
                this.meshes[i].material.uniforms['uIndexMap'].value = this.index_texture;
                this.meshes[i].draw(this.camera);
            }
        
            //console.log("finish all pass");
        }
    }
}