use LinearAlgebra;
use Random;
use Time;

config const size = 2552;
config const numThreads: int = 4;

proc initializeMatrix(mat: [?D] real) {
  var randStream = new randomStream(real); // Create a random number generator
  for i in D.dim(1) {
    for j in D.dim(2) {
      mat[i, j] = randStream.getNext(); // Assign a random real number to each element
    }
  }
}

proc matMult(A: [?AD] real, B: [?BD] real, C: [?CD] real) {
  forall i in AD.dim(1) {
    forall j in BD.dim(2) {
      var sum: real = 0;
      for k in AD.dim(2) {
        sum += A[i, k] * B[k, j];
      }
      C[i, j] = sum;
    }
  }
}

proc main() {
  var A: [1..size, 1..size] real;
  var B: [1..size, 1..size] real;
  var C: [1..size, 1..size] real;

  initializeMatrix(A);
  initializeMatrix(B);

  here.maxTaskPar = numThreads;

  const startTime = getCurrentTime();
  matMult(A, B, C);
  const endTime = getCurrentTime();

  writeln("Time taken for matrix multiplication with ", numThreads, " threads: ", (endTime - startTime): real, " seconds");
}
