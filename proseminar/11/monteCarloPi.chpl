use Random;
use Time;

config const numThreads: int = 4;
config const numPoints: int = 1_000_000;

proc estimatePi(numPoints: int, numThreads: int): real {
  var totalInsideCircle: int;
  var randStreams: [1..numThreads] randomStream(real);

  forall tid in 1..numThreads with (+ reduce totalInsideCircle) {
    var insideCircle: int;
    var randStream = randStreams[tid];

    for point in 1..(numPoints/numThreads) {
      const x = randStream.getNext(): real;
      const y = randStream.getNext(): real;
      if (x*x + y*y <= 1.0) {
        insideCircle += 1;
      }
    }
    totalInsideCircle += insideCircle;
  }

  return 4.0 * totalInsideCircle / numPoints: real;
}

proc main() {
  const startTime = timeSinceEpoch().totalSeconds();

  if numThreads < 1 || numThreads > here.maxTaskPar {
    writeln("Number of threads must be between 1 and ", here.maxTaskPar);
    return;
  }

  const piEstimate = estimatePi(numPoints, numThreads);

  const endTime = timeSinceEpoch().totalSeconds();
  writeln("Estimated Pi: ", piEstimate);
  writeln("Time taken: ", (endTime - startTime): real, " seconds");
}

