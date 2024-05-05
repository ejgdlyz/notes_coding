import edu.princeton.cs.algs4.StdRandom;
import edu.princeton.cs.algs4.StdStats;
import edu.princeton.cs.algs4.WeightedQuickUnionUF;
import edu.princeton.cs.algs4.StdOut;

public class PercolationStats {
    private final static double CONFIDENCE_95 = 1.96;
    private int n;
    private int trials;
    private double[] meanVals;
    private int[] sites;
    private double sample_mean;
    private double sample_stddev ;

    // perform independent trials on an n-by-n grid
    public PercolationStats(int n, int trials) {
        if (n <= 0 || trials <= 0) {
            throw new IllegalArgumentException();
        }
        this.n = n;
        this.trials = trials;
        meanVals = new double[trials];
        sites = new int[n * n];

        run();

        sample_mean = StdStats.mean(meanVals);
        
        sample_stddev = trials == 1 ? Double.NaN : StdStats.stddev(meanVals);
    }

    private void shuffle() {
        int len = sites.length;
        for (int i = 0; i < len; ++i) {
            sites[i] = i + 1;
        }
        
        for (int i = len - 1; i >= 0; --i) {
            int j = StdRandom.uniformInt(i + 1);
            int tmp = sites[i];
            sites[i] = sites[j];
            sites[j] = tmp;
        }
    }

    private void run() {
        for (int i = 0; i < trials; ++i) {
            // open a blocked site randomly every time until percolation
            shuffle();
            Percolation percolation = new Percolation(n);
            int j = 0;
            for (; j < sites.length; ++j) {
                int col = sites[j] % n;
                int row = sites[j] / n;
                if (col == 0) {
                    col = n; 
                } else {
                    ++row;
                }
                percolation.open(row, col);
                if (percolation.percolates()) {
                    break;
                } 
            }
            double xi = (j + 1) * 1.0 / sites.length;
            meanVals[i] = xi;
        }
    }

    // sample mean of percolation threshold
    public double mean() {  
        return sample_mean;
    }

    // sample standard deviation of percolation threshold
    public double stddev() {
        return sample_stddev;
    }

    // low endpoint of 95% confidence interval
    public double confidenceLo() {
        return sample_mean - CONFIDENCE_95 * sample_stddev / Math.sqrt(trials);
    }

    // high endpoint of 95% confidence interval
    public double confidenceHi() {
        return sample_mean + CONFIDENCE_95 * sample_stddev / Math.sqrt(trials);
    }

   // test client (see below)
   public static void main(String[] args) {
        int n = Integer.parseInt(args[0]);
        int trials = Integer.parseInt(args[1]);

        PercolationStats ps = new PercolationStats(n, trials);
        double mean = ps.mean();
        double stddev = ps.stddev();
        double conf_high = ps.confidenceHi();
        double conf_low = ps.confidenceLo();
        StdOut.println("mean" + "\t\t\t"  + "= "+ mean);
        StdOut.println("stddev" + "\t\t\t" + "= " + stddev);
        StdOut.println("95% confidence interval = " + "[" + conf_low + ", " + conf_high + "]");
   }

}