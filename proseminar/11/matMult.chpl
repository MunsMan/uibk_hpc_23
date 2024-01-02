use LinearAlgebra;
use Random;
use Time;

config const size = 2552;
config const numThreads: int = 4;

proc initializeMatrix(ref mat: [?D] real) {
  var randStream = new randomStream(real); // Create a random number generator
  for i in D.dim(0) {
    for j in D.dim(1) {
      mat[i, j] = randStream.getNext(); // Assign a random real number to each element
    }
  }
}

proc matMult(A: [?AD] real, B: [?BD] real,ref C: [?CD] real) {
  forall i in AD.dim(0) {
    forall j in BD.dim(1) {
      var sum: real = 0;
      for k in AD.dim(1) {
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

  const startTime = timeSinceEpoch().totalSeconds();
  matMult(A, B, C);
  const endTime = timeSinceEpoch().totalSeconds();

  writeln("Time taken for matrix multiplication with ", numThreads, " threads: ", (endTime - startTime): real, " seconds");
}
