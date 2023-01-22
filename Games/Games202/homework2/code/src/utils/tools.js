function getRotationPrecomputeL(precompute_L, rotationMatrix){

	let SHRotateMatrix_3x3 = computeSquareMatrix_3by3(rotationMatrix);
	let SHRotateMatrix_5x5 = computeSquareMatrix_5by5(rotationMatrix);
	let result = [];
	for(let j = 0; j < 9; j++){
		result[j] = [];
	}
	for(let j = 0; j < 3; j++){
		let temp3 = [precompute_L[1][j], precompute_L[2][j], precompute_L[3][j]];
		let temp5 = [precompute_L[4][j], precompute_L[5][j], precompute_L[6][j], precompute_L[7][j], precompute_L[8][j]];
		temp3 = math.multiply(temp3, SHRotateMatrix_3x3);
		temp5 = math.multiply(temp5, SHRotateMatrix_5x5);

		result[0][j] = precompute_L[0][j];
		result[1][j] = temp3._data[0];
		result[2][j] = temp3._data[1];
		result[3][j] = temp3._data[2];
		result[4][j] = temp5._data[0];
		result[5][j] = temp5._data[1];
		result[6][j] = temp5._data[2];
		result[7][j] = temp5._data[3];
		result[8][j] = temp5._data[4];
	}
	return result;
}

function computeSquareMatrix_3by3(rotationMatrix){ // 计算方阵SA(-1) 3*3 
	
	// 1、pick ni - {ni}
	let n1 = [1, 0, 0, 0]; let n2 = [0, 0, 1, 0]; let n3 = [0, 1, 0, 0];

	// 2、{P(ni)} - A  A_inverse
	let p1 = SHEval(n1[0], n1[1], n1[2], 3);
	let p2 = SHEval(n2[0], n2[1], n2[2], 3);
	let p3 = SHEval(n3[0], n3[1], n3[2], 3);

	let mat = math.matrix([
		[p1[1], p1[2], p1[3]],
		[p2[1], p2[2], p2[3]],
		[p3[1], p3[2], p3[3]]
	]);

	let A_inverse = math.inv(mat);

	// 3、用 R 旋转 ni - {R(ni)}
	let rot_n1 = vec4.create();
	let rot_n2 = vec4.create();
	let rot_n3 = vec4.create();
	mat4.multiply(rot_n1, rotationMatrix, n1);
	mat4.multiply(rot_n2, rotationMatrix, n2);
	mat4.multiply(rot_n3, rotationMatrix, n3);
	
	// 4、R(ni) SH投影 - S
	
	let rp1 = SHEval(rot_n1[0], rot_n1[1], rot_n1[2], 3);
	let rp2 = SHEval(rot_n2[0], rot_n2[1], rot_n2[2], 3);
	let rp3 = SHEval(rot_n3[0], rot_n3[1], rot_n3[2], 3);
	let S = math.matrix([
		[rp1[1], rp1[2], rp1[3]],
		[rp2[1], rp2[2], rp2[3]],
		[rp3[1], rp3[2], rp3[3]]
	]);

	// 5、S*A_inverse
	
	return math.multiply(S, A_inverse);
}

function computeSquareMatrix_5by5(rotationMatrix){ // 计算方阵SA(-1) 5*5
	
	// 1、pick ni - {ni}
	let k = 1 / math.sqrt(2);
	let n1 = [1, 0, 0, 0]; let n2 = [0, 0, 1, 0]; let n3 = [k, k, 0, 0]; 
	let n4 = [k, 0, k, 0]; let n5 = [0, k, k, 0];

	// 2、{P(ni)} - A  A_inverse

	let p1 = SHEval(n1[0], n1[1], n1[2], 3);
	let p2 = SHEval(n2[0], n2[1], n2[2], 3);
	let p3 = SHEval(n3[0], n3[1], n3[2], 3);
	let p4 = SHEval(n4[0], n4[1], n4[2], 3);
	let p5 = SHEval(n5[0], n5[1], n5[2], 3);

	let mat = math.matrix([
		[p1[4], p1[5], p1[6], p1[7], p1[8]],
		[p2[4], p2[5], p2[6], p2[7], p2[8]],
		[p3[4], p3[5], p3[6], p3[7], p3[8]],
		[p4[4], p4[5], p4[6], p4[7], p4[8]],
		[p5[4], p5[5], p5[6], p5[7], p5[8]]
	]);

	let A_inverse = math.inv(mat);

	// 3、用 R 旋转 ni - {R(ni)}
	let rot_n1 = vec4.create();
	let rot_n2 = vec4.create();
	let rot_n3 = vec4.create();
	let rot_n4 = vec4.create();
	let rot_n5 = vec4.create();
	mat4.multiply(rot_n1, rotationMatrix, n1);
	mat4.multiply(rot_n2, rotationMatrix, n2);
	mat4.multiply(rot_n3, rotationMatrix, n3);
	mat4.multiply(rot_n4, rotationMatrix, n4);
	mat4.multiply(rot_n5, rotationMatrix, n5);

	// 4、R(ni) SH投影 - S
	
	let rp1 = SHEval(rot_n1[0], rot_n1[1], rot_n1[2], 3);
	let rp2 = SHEval(rot_n2[0], rot_n2[1], rot_n2[2], 3);
	let rp3 = SHEval(rot_n3[0], rot_n3[1], rot_n3[2], 3);
	let rp4 = SHEval(rot_n4[0], rot_n4[1], rot_n4[2], 3);
	let rp5 = SHEval(rot_n5[0], rot_n5[1], rot_n5[2], 3);

	let S = math.matrix([
		[rp1[4], rp1[5], rp1[6], rp1[7], rp1[8]],
		[rp2[4], rp2[5], rp2[6], rp2[7], rp2[8]],
		[rp3[4], rp3[5], rp3[6], rp3[7], rp3[8]],
		[rp4[4], rp4[5], rp4[6], rp4[7], rp4[8]],
		[rp5[4], rp5[5], rp5[6], rp5[7], rp5[8]]
	]);

	// 5、S*A_inverse
	
	return math.multiply(S, A_inverse);
}

function mat4Matrix2mathMatrix(rotationMatrix){

	let mathMatrix = [];
	for(let i = 0; i < 4; i++){
		let r = [];
		for(let j = 0; j < 4; j++){
			r.push(rotationMatrix[i*4+j]);
		}
		mathMatrix.push(r);
	}
	return math.matrix(mathMatrix)

}

function getMat3ValueFromRGB(precomputeL){

    let colorMat3 = [];
    for(var i = 0; i<3; i++){
        colorMat3[i] = mat3.fromValues( precomputeL[0][i], precomputeL[1][i], precomputeL[2][i],
										precomputeL[3][i], precomputeL[4][i], precomputeL[5][i],
										precomputeL[6][i], precomputeL[7][i], precomputeL[8][i]); 
	}
    return colorMat3;
}
