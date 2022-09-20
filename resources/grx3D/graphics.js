/*
    _____     __  __ 
   /  _   |  |  |    |
  |  | |  |  |     __/       
  |  -_|  |  |   -
   \___   |  |  |
    __ |  |  |  |
   |  ||  |  |  |
   \_____ /  |__|

Javascript/HTML 3D rendering engine
Gabriel Racz
09-23-2021
*/

/*
CONTROLS:
w + s --> Rotate about X axis
a + d --> Rotate about Y axis
q + e --> Rotate about Z axis

j + l --> Translate X position
i + k --> Translate Y position
u + o --> Translate Z position

  [   --> Cycle through colors
  ]   --> Toggle wire frame mode
*/

//  TODO:
//  -Create classes (model, vec2d, vec3d, matrix3d, ...)
//  -Reduce copy pasted code in loop function
//  -Combine all transformationManager matricies into one matrix that performs them all
//  -Create HUD elements on webpage indicating position, model name, color, ...
//  -Create file parser that interprets object files from real modelling programs

var g_VariableStrokeIntensity = true;        //Wireframe mode
var g_WireFrame = false;
var g_Angle;
const COLORS = [
    [237, 40, 76],
    [255, 136, 0],
    [40, 237, 149],
    [49, 192, 224],
    [85, 0, 255],
    [140, 255, 0],
    [223, 21, 330],
    [224, 215, 202],
    [60, 56, 54]
];

const PRED      = -1; 
const AMBER     = 1;
const GREEN     = 2;
const LBLUE     = 3;
const INDIGO    = 4;
const SLIME     = 5;
const PINK      = 6
const ORANGE    = 7;
const BLACK     = 8;
const LGREY     = 9;
const DGREY     = 10;



var g_VariableStrokeIntensity = true;        //Wireframe mode
var g_WireFrame = false;
var g_Angle;

var model = {};

class Model {
    constructor(initX, initY, initZ, loader){
        // triangles 
    }
}

function main(){
    //Get canvas data
    var canvas = document.querySelector('canvas');
    canvas.focus();
    canvas.width = window.innerWidth*0.6;
    canvas.height = canvas.width*2/3;
    var ctx = canvas.getContext("2d");      //Used to manipulate shapes

    document.getElementById("fileupload").addEventListener("change", LoadObjectFile);

    //Init vars
    let init_x = 0;
    let init_y = 0;
    let init_z = 0;
    let size = 200;
	let d  = 1000;
    let fill_colour = COLORS[LGREY];
    let stroke_colour = COLORS[LGREY];
    // g_VariableStrokeIntensity = false;	

    var camera = [0, 0, d, 1];

    var model_data = ConstructCube(init_x, init_y, init_z, size);
    model = {
        mesh: model_data[0],        //Contains vertex vector data
        triangles: model_data[1],    //Stores mesh indexes to construct each triangle
        centerx: init_x,
        centery: init_y,
        centerz: init_z
    };


    model.mesh = RotateX3D(model.mesh, Math.PI/2, init_x, init_y, init_z);      //Make shape right side up do to reverse of Y axis

    var transformationManager = {
        rotx: 0,
        roty: 0,
        rotz: 0,
        rotlock: false,
        transx: 0,
        transy: 0,
        transz: 0
    };


    var modelManager = {value:0};
    var colorManager = {index:9};
    addKeydownHandler(canvas, modelManager, transformationManager, colorManager);
    addKeyupHandler(canvas,transformationManager)

    var angle = 0.05;
    var dist = 3;
    var model_catalogue = {
        1: ConstructTetrahedron,
        2: ConstructPyramid,
        3: ConstructOctahedron,
        4: ConstructCube,
        5: ConstructIcosahedron,
        6: ConstructHeart
    }
    function loop(){
        //Perform Rotations
        if(transformationManager.rotx){
            model.mesh = RotateX3D(model.mesh, angle * transformationManager.rotx, model.centerx, model.centery, model.centerz);
        }
        if(transformationManager.roty){
            model.mesh = RotateY3D(model.mesh, angle * transformationManager.roty, model.centerx, model.centery, model.centerz);
        }
        if(transformationManager.rotz){
            model.mesh = RotateZ3D(model.mesh, angle * transformationManager.rotz, model.centerx, model.centery, model.centerz);
        }

        //Perform Translations
        if(transformationManager.transx){
            model.mesh = TranslateX3D(model.mesh, dist * transformationManager.transx);
            model.centerx += dist * transformationManager.transx;
        }if(transformationManager.transy){
            model.mesh = TranslateY3D(model.mesh, dist * transformationManager.transy);
            model.centery += dist * transformationManager.transy;
        }

        if(modelManager.value){
            model_data = model_catalogue[modelManager.value](model.centerx, model.centery, model.centerz, size);
            var new_model = {
                mesh: model_data[0],
                triangles: model_data[1],
                centerx: model.centerx,
                centery: model.centery,
                centerz: model.centerz
            };
            model = new_model;
            model.mesh = RotateX3D(model.mesh, Math.PI, init_x, init_y, init_z);
            modelManager.value = 0;

        }
        fill_colour = COLORS[colorManager.index];
        stroke_colour = COLORS[colorManager.index];
        DrawMesh(model.mesh, model.triangles, camera, ctx, fill_colour, stroke_colour);
    }
    setInterval(loop, 30);
}

function LoadObjectFile(event){
    //Init file reader
    reader = new FileReader();
    reader.readAsText(event.target.files[0]);
    reader.onload = readSuccess;

    let mesh = [];
    let triangles = [];

    //Construct object
    function readSuccess(){
        let obj = reader.result.split("\n");

        let scale = 1;

        for(let i = 0; i < obj.length; i++){
            let line = obj[i];
            if(line.startsWith("v")){        //Vertex (mesh point)
                let s = line.split(" ");        //Split at spaces and put the 3 following coordinates into a point
                let v = [Number(s[1] * scale), Number(s[2] * scale), Number(s[3] * scale), 1];
                mesh.push(v);
            }else if(line.startsWith("f")){      //Face (triangle)
                let s = line.split(" ");
                let t = [Number(s[1])-1, Number(s[2])-1, Number(s[3])-1];
                triangles.push(t);
            }
       }

       document.getElementById("vertex-count").innerHTML = mesh.length;
       document.getElementById("triangle-count").innerHTML = triangles.length;
       model.mesh = mesh;
       model.triangles = triangles;
       model.centerx = 0;
       model.centery = 0;
       model.centerz = 0;
       model.mesh = RotateZ3D(model.mesh, Math.PI, 0, 0, 0);
       console.log("obj created")
    }
}

function DrawMesh(mesh, triangles, camera, ctx, fill_colour = [200, 200, 200], stroke_colour = fill_colour){
    let screen_width = ctx.canvas.width;
    let screen_height = ctx.canvas.height;
    ctx.clearRect(0, 0, screen_width, screen_height);

    let proj = [];
    for(let i = 0; i < mesh.length; i++){
        proj.push(PerspectiveProjection(mesh[i], camera[2]));
    }

    let sorted_triangles = triangles;
    sorted_triangles.sort(function(a, b){
            let zavga = (mesh[a[0]][2] + mesh[a[1]][2] + mesh[a[2]][2])/3;
            let zavgb = (mesh[b[0]][2] + mesh[b[1]][2] + mesh[b[2]][2])/3;
            return zavga - zavgb;
        }
    );

    ctx.beginPath();
    //ctx.strokeStyle = stroke_colour;
    for(let i = 0; i < sorted_triangles.length; i++){
        let t = sorted_triangles[i];                        //current triangle
        let normal = CalculateNormal(t, mesh);              //Normal that points outwards

        //Dot product represents the difference between the two angles formed by the triangle and the camera (-1 to 1)
        let lightDirection = [0, 0, -1, 1];     //This light direction needs to be changed to reflect the camera's position in the world

        let light_intensity = DotProduct(normal, lightDirection);
        light_intensity = Math.max(light_intensity, 0.1);
        
        let camera_line = Vector_Sub(mesh[t[0]], camera);
        let dotProduct = DotProduct(normal, camera_line);

        //When the dot product is positive, the normal is pointed towards the camera's line of sight and the triangle should be drawn
        if(dotProduct > 0){
            let path =  new Path2D();

            //Line to each triangle vertex
            path.moveTo(proj[t[0]][0] + screen_width/2, proj[t[0]][1] + screen_height/2);
            path.lineTo(proj[t[1]][0] + screen_width/2, proj[t[1]][1] + screen_height/2);
            path.lineTo(proj[t[2]][0] + screen_width/2, proj[t[2]][1] + screen_height/2);
            path.lineTo(proj[t[0]][0] + screen_width/2, proj[t[0]][1] + screen_height/2);

            if(g_WireFrame){
                ctx.fillStyle = "rgb(0, 0, 0)";
                ctx.fill(path);
                ctx.strokeStyle = "rgb(" + stroke_colour[0] + ", " + stroke_colour[1] + ", " + stroke_colour[2] + ", 255)";
                ctx.stroke(path);
                continue;
            }

            ctx.fillStyle = "rgb(" + light_intensity*fill_colour[0] + ", " + light_intensity*fill_colour[1] + ", " + light_intensity*fill_colour[2] + ", 255)";
            if(g_VariableStrokeIntensity){
                ctx.strokeStyle = "rgb(" + light_intensity*stroke_colour[0] + ", " + light_intensity*stroke_colour[1] + ", " + light_intensity*stroke_colour[2] + ", 255)";
            }else{
                ctx.strokeStyle = "rgb(" + stroke_colour[0] + ", " + stroke_colour[1] + ", " + stroke_colour[2] + ", 255)";
            }
            ctx.fill(path);
            ctx.stroke(path);

        }
    }
}

function DotProduct(vector1, vector2){
    var dp = 0;
    for(let i = 0; i < vector1.length - 1; i++){
        dp += vector1[i] * vector2[i];
    }
    return dp;
}

function Vector_Sub(vector1, vector2){
    let result = [];
    for(let i = 0; i < vector1.length; i++){
        result.push(vector1[i] - vector2[i]);
    }
    return result;
}

function Vector_Add(vector1, vector2){
    let result = [];
    for(let i = 0; i < vector1.length; i++){
        result.push(vector1[i] - vector2[i]);
    }
    return result;
}


function NormalizeVector(vector){
    let normalized = [];
    let sqr_sum = 0;
    for(let i = 0; i < vector.length; i++){
        sqr_sum += vector[i] * vector[i];
    }
    let length = Math.sqrt(sqr_sum);
    for(let i = 0; i < vector.length; i++){
        normalized.push(vector[i]/length);
    }
    normalized.push(1);
    return normalized;
}

function CalculateNormal(triangle, mesh){
    let normal = [];
    let line1 = [];
    let line2 = [];

    //  0
    //  | \
    //  |  \  
    //  |   \
    //  1_____2
    //  
    
    //Find the vectors representing lines from: (v1 -> v0) and (v1 -> v2)
    //Did it in this order to have a normal that is facing out, not in
    line1[0] = mesh[triangle[0]][0] - mesh[triangle[1]][0];     //X    
    line1[1] = mesh[triangle[0]][1] - mesh[triangle[1]][1];     //Y    
    line1[2] = mesh[triangle[0]][2] - mesh[triangle[1]][2];     //Z

    line2[0] = mesh[triangle[2]][0] - mesh[triangle[1]][0];     
    line2[1] = mesh[triangle[2]][1] - mesh[triangle[1]][1];     
    line2[2] = mesh[triangle[2]][2] - mesh[triangle[1]][2];     

        


    //Cross product formula finds normal perpendicular to line1 and line2
    normal[0] = line1[1] * line2[2] - line1[2] * line2[1];
    normal[1] = line1[2] * line2[0] - line1[0] * line2[2];
    normal[2] = line1[0] * line2[1] - line1[1] * line2[0];

    //Normalize the normal to a unit vector
    normal = NormalizeVector(normal);

    return normal;
}

//      MODEL CONSTRUCTION
//  -Point (x, y, z) defines center point
//  -Size defines the general distance from center point to verticies
//  -Mesh is an array of length 4 vectors representing each vertex
//  -Triangles is an array of length 3 arrays containing the indicies of the triangle's verticies in the mesh
//  -Triangle verticies are always defined in counterclockwise order
//  -Returns tuple containing mesh and triangle arrays
function ConstructCube(x, y, z, size){
    let v1 = [x - size/2, y + size/2, z + size/2, 1];
    let v2 = [x - size/2, y - size/2, z + size/2, 1];
    let v3 = [x + size/2, y - size/2, z + size/2, 1];
    let v4 = [x + size/2, y + size/2, z + size/2, 1];
    let v5 = [x + size/2, y + size/2, z - size/2, 1];
    let v6 = [x - size/2, y + size/2, z - size/2, 1];
    let v7 = [x - size/2, y - size/2, z - size/2, 1];
    let v8 = [x + size/2, y - size/2, z - size/2, 1];
    let mesh = [v1, v2, v3, v4, v5, v6, v7, v8];        //Specifies verticies of the cube

    let triangles = [                                   //Specifies verticies for each triangle
        [0, 1, 2], 
        [0, 2, 3], 
        [3, 2, 7],
        [3, 7, 4],
        [4, 7, 6],
        [4, 6, 5],
        [5, 6, 1],
        [5, 1, 0],
        [3, 4, 5],
        [3, 5, 0],
        [1, 6, 7],
        [1, 7 ,2]
    ];
    return [mesh, triangles];
}


function ConstructHeart(x, y, z, size){
    let v1 = [x, y + size, z, 1];
    let v2 = [x - size, y, z + size/2, 1];
    let v3 = [x + size/2, y, z + size/2, 1];
    let v4 = [x + size, y, z - size/2, 1];
    let v5 = [x - size/2, y, z - size/2, 1];
    let v6 = [x, y - size, z, 1];
    let v7 = [x - size/2, y + size*2/3, z + size/4, 1];
    let v8 = [x + size/2, y + size*2/3, z - size/4, 1];
    let v9 = [x, y, z, 1];
    let mesh = [v1, v2, v3, v4, v5, v6, v7, v8, v9];

    let triangles = [
        [5, 2, 1],
        [5, 1, 4],
        [5, 4, 3],
        [5, 3, 2],
        [6, 1, 2],
        [6, 2, 4],
        [6, 4, 1],
        [7, 2, 3],
        [7, 3, 4],
        [7, 4, 2]
    ];

    return [mesh, triangles];
}

function ConstructPyramid(x, y, z, size){
    let v1 = [x, y + (size/2), z, 1];
    let v2 = [x - size/2, y - size/2, z + size/2, 1];
    let v3 = [x + size/2, y - size/2, z + size/2, 1];
    let v4 = [x + size/2, y - size/2, z - size/2, 1];
    let v5 = [x - size/2, y - size/2, z - size/2, 1];
    let mesh = [v1, v2, v3, v4, v5];

    let triangles = [
        [0, 1, 2],
        [0, 2, 3],
        [0, 3, 4],
        [0, 4, 1],
        [1, 4, 3],
        [1, 3, 2]
    ];
    return [mesh, triangles];
}

function ConstructTetrahedron(x, y, z, size){
    let a = size/2;

    let v1 = [x + a, y + a, z + a, 1];
    let v2 = [x - a, y + a, z - a, 1];
    let v3 = [x - a, y - a, z + a, 1];
    let v4 = [x + a, y - a, z - a, 1];
    let mesh = [v1, v2, v3, v4];

    let triangles = [
        [3, 1, 0],
        [3, 2, 1],
        [2, 3, 0],
        [1, 2, 0]
    ];

    return[mesh, triangles];
}

function ConstructOctahedron(x, y, z, size){
    let v1 = [x, y + (size/2)/0.7071, z, 1];
    let v2 = [x - size/2, y, z + size/2, 1];
    let v3 = [x + size/2, y, z + size/2, 1];
    let v4 = [x + size/2, y, z - size/2, 1];
    let v5 = [x - size/2, y, z - size/2, 1];
    let v6 = [x, y - (size/2)/0.7071, z, 1];
    let mesh = [v1, v2, v3, v4, v5, v6];

    let triangles = [
        [0, 1, 2],
        [0, 2, 3],
        [0, 3, 4],
        [0, 4, 1],
        [5, 2, 1],
        [5, 1, 4],
        [5, 4, 3],
        [5, 3, 2]
    ];
    return [mesh, triangles];
}

function ConstructIcosahedron(x, y, z, size){
    let v1 = [0.000  * size/2 + x,  1.000 * size/2 + y, 0.000  * size/2 + z, 1];
    let v2 = [0.894  * size/2 + x,  0.447 * size/2 + y, 0.000  * size/2 + z, 1];
    let v3 = [0.276  * size/2 + x,  0.447 * size/2 + y, 0.851  * size/2 + z, 1];
    let v4 = [-0.724 * size/2 + x,  0.447 * size/2 + y, 0.526  * size/2 + z, 1];
    let v5 = [-0.724 * size/2 + x,  0.447 * size/2 + y, -0.526 * size/2 + z, 1];
    let v6 = [0.276  * size/2 + x,  0.447 * size/2 + y, -0.851 * size/2 + z, 1];
    let v7 = [0.724  * size/2 + x, -0.447 * size/2 + y, 0.526  * size/2 + z, 1];
    let v8 = [-0.276 * size/2 + x, -0.447 * size/2 + y, 0.851  * size/2 + z, 1];
    let v9 = [-0.894 * size/2 + x, -0.447 * size/2 + y, 0.000  * size/2 + z, 1];
    let v10 = [-0.276* size/2 + x, -0.447 * size/2 + y, -0.851 * size/2 + z, 1];
    let v11 = [0.724 * size/2 + x, -0.447 * size/2 + y, -0.526 * size/2 + z, 1];
    let v12 = [0.000 * size/2 + x, -1.000 * size/2 + y, 0.000  * size/2 + z, 1];
    let mesh = [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12];

    let triangles = [
        [11, 9 , 10 ],
        [11, 8 , 9 ],
        [11, 7 , 8 ],
        [11, 6 , 7 ],
        [11,10 , 6 ],
        [0 , 5 , 4 ],
        [0 , 4 , 3 ],
        [0 , 3 , 2 ],
        [0 , 2 , 1 ],
        [0 , 1 , 5 ],
        [10, 9 ,  5 ],
        [9 , 8 , 4 ],
        [8 , 7 , 3 ],
        [7 , 6 , 2 ],
        [6 ,10 , 1 ],
        [5 , 9 , 4 ],
        [4 , 8 , 3 ],
        [3 , 7 , 2 ],
        [2 , 6 , 1 ],
        [1 ,10 , 5 ]
    ];

    return [mesh, triangles];

}


function TranslateX3D(matrix, direction){
    let result = [];
    if(matrix[0].length == undefined){
        [matrix[i][0] + direction, matrix[i][1], matrix[i][2], 1];
    }else{
        for(let i = 0; i < matrix.length; i++){
            translated_vector = [matrix[i][0] + direction, matrix[i][1], matrix[i][2], 1]; 
            result.push(translated_vector);
        }
    }
    return result;
}

function TranslateY3D(matrix, direction){
    let result = [];
    if(matrix[0].length == undefined){
        [matrix[i][0], matrix[i][1] + direction, matrix[i][2], 1];
    }else{
        for(let i = 0; i < matrix.length; i++){
            translated_vector = [matrix[i][0], matrix[i][1] + direction, matrix[i][2], 1]; 
            result.push(translated_vector);
        }
    }
    return result;
}

function TranslateZ3D(matrix, direction){
    let result = [];
    if(matrix[0].length == undefined){
        [matrix[i][0], matrix[i][1], matrix[i][2] + direction, 1];
    }else{
        for(let i = 0; i < matrix.length; i++){
            translated_vector = [matrix[i][0], matrix[i][1], matrix[i][2] + direction, 1]; 
            result.push(translated_vector);
        }
    }
    return result;
}


//      ROTATIONS
//  -Rotate given matrix (or vector) around one of its axes
//  -Angle phi in radians
//  -Point (a, b, c) is the center of rotation
function RotateX3D(matrix, phi, a, b, c){
    let result = [];
    let rotation = [
        [1, 0, 0, 0],
        [0, Math.cos(phi), -Math.sin(phi), -b*Math.cos(phi) + c*Math.sin(phi) + b],
        [0, Math.sin(phi), Math.cos(phi), -b*Math.sin(phi) - c*Math.cos(phi) + c],
        [0, 0, 0, 1]
    ];

    if(matrix[0].length == undefined){
        result = MatrixVectorProduct(rotation, matrix);             //Matrix is actually a vector
    }else{
        for(let i = 0; i < matrix.length; i++){
            result.push(MatrixVectorProduct(rotation, matrix[i]));  //Rotate each vector in the matrix
        }   
    }

    return result;
}

function RotateY3D(matrix, phi, a, b, c){
    let result = [];
    let rotation = [
        [Math.cos(phi),  0, Math.sin(phi), -a*Math.cos(phi) - c*Math.sin(phi) + a],
        [0,              1,      0,                  0],
        [-Math.sin(phi), 0, Math.cos(phi), a*Math.sin(phi) - c*Math.cos(phi) + c],
        [0,              0,      0,                  1]
    ];

    if(matrix[0].length == undefined){
        result = MatrixVectorProduct(rotation, matrix);             
    }else{
        for(let i = 0; i < matrix.length; i++){
            result.push(MatrixVectorProduct(rotation, matrix[i]));  
        }   
    }
    return result;
}

function RotateZ3D(matrix, phi, a, b, c){
    let result = [];
    let rotation = [
        [Math.cos(phi), -Math.sin(phi), 0, -a*Math.cos(phi) + b*Math.sin(phi) + a],
        [Math.sin(phi), Math.cos(phi), 0, -a*Math.sin(phi) - b*Math.cos(phi) + b],
        [0, 0, 1, 0],
        [0, 0, 0, 1]
    ];

    if(matrix[0].length == undefined){
        result = MatrixVectorProduct(rotation, matrix);           
    }else{
        for(let i = 0; i < matrix.length; i++){
            result.push(MatrixVectorProduct(rotation, matrix[i]));
        }   
    }

    return result;
}

//Create 2D projection of 3D vector from a distance d to be able to draw to the 2D screen space
function PerspectiveProjection(vector, d){
    let result = [];
    result.push(vector[0] / (1 - vector[2]/d));     //Divide each homogeneous coordinate by (1 - z/d)
    result.push(vector[1] / (1 - vector[2]/d));
    return result;
}

//      MATRIX/VECTOR ALGEBRA
// -Multiply arg1 by arg2
function MatrixMultiplication(A, B){
    
    //Check if matrix multiplication is consistent
    if(B.length != A[0].length){
        return null;
    }
    
    //Init matrix
    let result = [];
    for(let i = 0; i < A.length; i++){
        result.push([]);
    }
    
    for(let i = 0; i < B[0].length; i++){           //For every column of B
        for(let j = 0; j < A.length; j++){          //For every row of A
            let sum = 0;
            for(let k = 0; k < A[0].length; k++){   //For every element in A
                sum += A[j][k] * B[k][i];           //Sum the row
            }
            result[j].push(sum);                    //Push the new row to result
        }
    }
    return result;
}

function MatrixVectorProduct(matrix, vector){
    if(vector.length != matrix[0].length){
        return NaN;
    }

    let result = [];
    for(let i = 0; i < matrix.length; i++){
        let sum = 0;
        for(let j = 0; j < matrix[0].length; j++){
            sum += matrix[i][j] * vector[j];
        }
        result.push(sum);
    }
    return result;
}

function PrintMatrix(matrix){
    for(let m = 0; m < matrix.length; m++){
        console.log(matrix[m]);
    }
}



//      EVENT HANDLERS
function addKeydownHandler(canvas, modelManager, transformationManager, colorManager){
    canvas.addEventListener("keydown", function HandleKeyDown(e){
        switch(e.key){
            case 'w':
                transformationManager.rotx = 1;
                break;
            case 's':
                transformationManager.rotx = -1;
                break;
            case 'd':
                transformationManager.roty = 1;
                break;
            case 'a':
                transformationManager.roty = -1;
                break;
            case 'e':
                transformationManager.rotz = 1;
                break;
            case 'q':
                transformationManager.rotz = -1;
                break;
            case "Shift":
                transformationManager.rotlock = !transformationManager.rotlock;
                if(!transformationManager.rotlock){
                    transformationManager.rotx = 0;
                    transformationManager.roty = 0;
                    transformationManager.rotz = 0;
                }
                break;
            case "l":
                transformationManager.transx = 1;
                break;
            case "j":
                transformationManager.transx = -1;
                break;
            case "k":
                transformationManager.transy = 1;
                break;
            case "i":
                transformationManager.transy = -1;
                break;
            case ".":
                camera[2] += 100;
                break;
            case ",":
                camera[2] -= 100;
                break;
            case '1':
                modelManager.value = 1;
                break;
            case '2':
                modelManager.value = 2;
                break;
            case '3':
                modelManager.value = 3;
                break;
            case '4':
                modelManager.value = 4;
                break;
            case '5':
                modelManager.value = 5;
                break;
            case '6':
                modelManager.value = 6;
                break;
            case ']':
                g_WireFrame = (g_WireFrame + 1) % 2;
                break;
            case '[':
                colorManager.index = (colorManager.index + 1) % COLORS.length;

                break;
        }
    });
}

function addKeyupHandler(canvas, transformationManager){
    canvas.addEventListener("keyup", function HandleKeyUp(e){
        switch(e.key){
            case 'w':
                if(!transformationManager.rotlock)
                    transformationManager.rotx =0;
                break;
            case 's':
                if(!transformationManager.rotlock)
                    transformationManager.rotx =0;
                break;
            case 'd':
                if(!transformationManager.rotlock)
                    transformationManager.roty =0;
                break;
            case 'a':
                if(!transformationManager.rotlock)
                    transformationManager.roty = 0;
                break;
            case 'e':
                if(!transformationManager.rotlock)
                    transformationManager.rotz = 0;
                break;
            case 'q':
                if(!transformationManager.rotlock)
                    transformationManager.rotz = 0;
                break;
            case 'l':
                transformationManager.transx = 0;
                break;
            case 'j':
                transformationManager.transx = 0;
                break;
            case 'i':
                transformationManager.transy = 0;
                break;
            case 'k':
                transformationManager.transy = 0;
                break;
            
            
        }
    }); 
}

//Run the program
main();
