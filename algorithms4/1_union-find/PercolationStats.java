import edu.princeton.cs.algs4.StdRandom;
import edu.princeton.cs.algs4.StdStats;
import edu.princeton.cs.algs4.WeightedQuickUnionUF;
import edu.princeton.cs.algs4.StdOut;
import java.lang.IllegalArgumentException;

public class PercolationStats {
    private int n;
    private int trials;
    private double[] meanVals;
    private int[] sites;

    // perform independent trials on an n-by-n grid
    public PercolationStats(int n, int trials) {
        if (n <= 0 || trials <= 0) {
            throw new IllegalArgumentException();
        }
        this.n = n;
        this.trials = trials;
        meanVals = new double[trials];
        sites = new int[n * n];
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

    public void run() {
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
        double x = 0.0;
        for (int i = 0; i < meanVals.length; ++i) {
            x += meanVals[i];
        }   
        return x / meanVals.length;
    }

    // sample standard deviation of percolation threshold
    public double stddev() {
        double s = 0.0;
        double mean_val = mean();
        for (int i = 0; i < meanVals.length; ++i) {
            s += (meanVals[i] - mean_val) * (meanVals[i] - mean_val);
        }
        return Math.sqrt(s / meanVals.length);
    }

    // low endpoint of 95% confidence interval
    public double confidenceLo() {
        double xhat = mean();
        double s = stddev();
        return xhat - 1.96 * s / Math.sqrt(trials);
    }

    // high endpoint of 95% confidence interval
    public double confidenceHi() {
        double xhat = mean();
        double s = stddev();
        return xhat + 1.96 * s / Math.sqrt(trials);
    }

   // test client (see below)
   public static void main(String[] args) {
        int n = Integer.parseInt(args[0]);
        int trials = Integer.parseInt(args[1]);

        PercolationStats ps = new PercolationStats(n, trials);
        ps.run();
        double mean = ps.mean();
        double stddev = ps.stddev();
        double conf_high = ps.confidenceHi();
        double conf_low = ps.confidenceLo();
        StdOut.println("mean" + "\t\t\t"  + "= "+ ps.mean());
        StdOut.println("stddev" + "\t\t\t" + "= " +ps.stddev());
        StdOut.println("95% confidence interval = " + "[" + conf_low + ", " + conf_high + "]");
   }

}