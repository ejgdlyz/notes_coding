import edu.princeton.cs.algs4.WeightedQuickUnionUF;

public class Percolation {
    private final int[] directions = {-1, 0, 0, 1, 1, 0, 0, -1};
    private int rows = 0;
    private int cols = 0;
    private boolean[][] grid;
    private int openSites = 0;
    private WeightedQuickUnionUF uf;
    // for backwash,
    private WeightedQuickUnionUF backwashUf;
    private int virtualNode1, virtualNode2;

    // creates n-by-n grid, with all sites initially blocked
    public Percolation(int n) {
        if (n <= 0) {
            throw new IllegalArgumentException();
        }
        openSites = 0;
        rows = cols = n;
        grid = new boolean[rows][cols];
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                grid[i][j] = false;
            }
        }
        uf = new WeightedQuickUnionUF(rows * cols + 2);
        backwashUf = new WeightedQuickUnionUF(rows * cols + 2);
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
            throw e;
        }
        row--;
        col--;
        grid[row][col] =  true;
        if (row == 0) {
            // virtual node 1
            uf.union(virtualNode1, row * rows + col);
            backwashUf.union(virtualNode1, row * rows + col);
             
        }
        if (row == rows - 1) {
            // vurtual node 2
            uf.union(virtualNode2, row * rows + col);
            // backwashUf does not record this situation
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
        return grid[row][col] != false;
    }

    // is the site (row, col) full?
    public boolean isFull(int row, int col) {
        if (!isOpen(row, col)) {
            return false;
        }
        row--;
        col--;
        // return Connected(virtualNode1, row * cols + col);
        return backwashUf.find(virtualNode1) == backwashUf.find(row * cols + col);
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

    private boolean Connected (int p, int q) {
        return uf.find(p) == uf.find(q);
    }

    // 4 directions union 
    private void Union(int row, int col) {
        int p = row * rows + col;
        for (int i = 0; i < 8; i += 2) {
            int nx = row + directions[i];
            int ny = col + directions[i + 1];
            if (0 <= nx && nx < rows && 0 <= ny && ny < cols) {
                if (grid[nx][ny] == true) {
                    uf.union(p, nx * rows + ny);
                    backwashUf.union(p, nx * rows + ny);
                    // if (percolates()) {
                    //     return;
                    // }
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
        Percolation p = new Percolation(3);
        p.open(1, 1);
        p.open(1, 3);
        p.open(2, 3);
        p.open(3, 3);
        p.open(3, 1);
        p.open(2, 1);
        System.out.println(p.percolates());
        System.out.println(p.isFull(3, 1));
        System.out.println(p.isFull(3, 1));

    }
}