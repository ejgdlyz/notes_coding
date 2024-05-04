import edu.princeton.cs.algs4.StdRandom;
import edu.princeton.cs.algs4.StdStats;
import edu.princeton.cs.algs4.WeightedQuickUnionUF;
import java.lang.IllegalArgumentException;

public class Percolation {
    private int rows = 0;
    private int cols = 0;
    private int[][] grid;
    private int openSites = 0;
    private WeightedQuickUnionUF uf;
    private static int[] directions = {-1, 0, 0, 1, 1, 0, 0, -1};
    private int virtualNode1, virtualNode2;

    // creates n-by-n grid, with all sites initially blocked
    public Percolation(int n) {
        if (n <= 0) {
            throw new IllegalArgumentException();
        }
        openSites = 0;
        rows = cols = n;
        grid = new int[rows][cols];
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                grid[i][j] = 0;
            }
        }
        uf = new WeightedQuickUnionUF(rows * cols + 2);
        virtualNode1 = rows * cols;
        virtualNode2 = rows * cols + 1;
    }

    // opens the site (row, col) if it is not open already
    public void open(int row, int col) {
        try {
            if (isOpen(row, col)) {
                return;
            }
        } catch (IllegalArgumentException e) {
            System.out.println(row + " " + col);
            throw e;
        }
        row--;
        col--;
        grid[row][col] =  1;
        if (row == 0) {
            // virtual node 1
            uf.union(virtualNode1, row * rows + col);
             
        } else if (row == rows - 1){
            // vurtual node 2
            uf.union(virtualNode2, row * rows + col);
        }
        Union(row, col);
        ++openSites;
    }

    // is the site (row, col) open?
    public boolean isOpen(int row, int col) {
        row--; 
        col--;
        try {
            isValid(row, col);
        } catch (IllegalArgumentException e) {
            throw e;
        }
        return grid[row][col] != 0;
    }

    // is the site (row, col) full?
    public boolean isFull(int row, int col) {
        row--;
        col--;
        try {
            isValid(row, col);
        } catch (IllegalArgumentException e) {
            throw e;
        }
        if (grid[row][col] == 0) {
            return false;
        }
        return Connected(virtualNode1, row * cols + col);
    }

    // returns the number of open sites
    public int numberOfOpenSites() {
        return openSites;
    }

    // does the system percolate?
    public boolean percolates() {
        return Connected(virtualNode1, virtualNode2);
    }

    private boolean isValid(int row, int col) {
        if (row < 0 || col < 0 || row >= rows || col >= cols) {
            throw new IllegalArgumentException();
        }
        return true;
    }

    public boolean Connected (int p, int q) {
        return uf.find(p) == uf.find(q);
    }

    // 4 directions union 
    public void Union(int row, int col) {
        int p = row * rows + col;
        for (int i = 0; i < 8; i += 2) {
            int nx = row + directions[i];
            int ny = col + directions[i + 1];
            if (0 <= nx && nx < rows && 0 <= ny && ny < cols) {
                if (grid[nx][ny] == 1) {
                    uf.union(p, nx * rows + ny);
                    if (percolates()) {
                        return;
                    }
                }
                
            }
        }
    }   

    public String toString() {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                System.out.print(Connected(virtualNode1, i * cols + j) + "\t");
            }
            System.out.println();
        }
        // System.out.println(grid.toString());
        return "";
    }

    // test client (optional)
    public static void main(String[] args) {
        Percolation p = new Percolation(8);
        int row = 1;
        for(int col = 3; col <= 5; ++col) {
            p.open(row, col);
        }
        boolean ret = p.isOpen(row, 3);
        System.out.println(ret);
        // p.toString();

        row = 2;
        for(int col = 4; col <= 8; ++col) {
            p.open(row, col);
        }
        ret = p.isFull(row, 8);
        System.out.println(ret);
        p.toString();

        row = 3;
        for(int col = 6; col <= 7; ++col) {
            p.open(row, col);
        }
        
        row = 4;
        for(int col = 6; col <= 8; ++col) {
            p.open(row, col);
        }

        row = 5;
        for(int col = 6; col <= 7; ++col) {
            p.open(row, col);
        }

        row = 6;
        for(int col = 7; col <= 8; ++col) {
            p.open(row, col);
        }

        row = 7;
        for(int col = 5; col <= 8; ++col) {
            p.open(row, col);
        }

        row = 8;
        p.open(row, 6);

        System.out.println(p.percolates());
        p.toString();

    }
}