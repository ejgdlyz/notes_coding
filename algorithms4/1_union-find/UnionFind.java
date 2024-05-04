import edu.princeton.cs.algs4.StdIn;
import edu.princeton.cs.algs4.StdOut;
import edu.princeton.cs.algs4.StdRandom;

public class UnionFind {
    public static void main(String[] args) throws Exception {
        int N = System.in.read();
        UF uf = new UF(N);
        while (!StdIn.isEmpty()) {
            int p = StdIn.readIn();
            int q = StdIn.readIn();
            if (!uf.connected(p, q)) {
                uf.union(p, q);
                StdOut.println(p + " " + q);
            }
        }
        System.out.println("Hello, World!");
    }

    public class QuickFindUF {
        private int[] id;

        public QuickFindUF(int N) {
            id = new int[N];
            for (int i = 0; i < N; ++i) {
                id[i] = i;
            }
        }

        public boolean connected(int p, int q) {
            return id[p] == id[q];
        }

        public void union(int p, int q) {
            int pid = id[p];
            int qid = id[q];
            for (int i = 0; i < id.length; ++i) {
                if (id[i] == pid) {
                    id[i] = qid;
                }
            }
        }
    }

    public class QuickUnionUF {
        private int[] id;

        public QuickUnionUF(int N) {
            id = new int[N];
            for (int i = 0; i < N; ++i) {
                id[i] = i;
            }
        }

        public int root(int i) {
            while (i != id[i]) {
                i = id[i];
            }
            return i;
        }

        public boolean connected(int p, int q) {
            return root(p) == root(q);
        }

        public void union(int p, int q) {
            int proot = root(p);
            int qroot = root(q);
            id[proot] = qroot;
        }
    }

    public class WeightedQuickUnionUF {
        private int[] id;
        private int[] sz; // 根 i 树中的对象数

        public WeightedQuickUnionUF(int N) {
            id = new int[N];
            sz = new int[N];

            for (int i = 0; i < N; ++i) {
                id[i] = i;
                sz[i] = 1;
            }
        }

        public int root(int i) {
            while (i != id[i]) {
                i = id[i];
            }
            return i;
        }

        public boolean connected(int p, int q) {
            return root(p) == root(q);
        }

        public void union(int p, int q) {
            int proot = root(p);
            int qroot = root(q);
            if (proot == qroot) {
                return;
            }
            if (sz[proot] < sz[qroot]) {
                id[proot] = qroot;
                sz[qroot] += sz[proot];
            } else {
                id[qroot] = proot;
                sz[proot] += sz[qroot];
            }
        }
    }

    public class PathCompressionWeightedQuickUnionUF {
        private int[] id;
        private int[] sz; // 记录根节点 i 中的对象数

        public PathCompressionWeightedQuickUnionUF(int N) {
            id = new int[N];
            sz = new int[N];

            for (int i = 0; i < N; ++i) {
                id[i] = i;
                sz[i] = 1;
            }
        }

        public int root(int i) {
            while (i != id[i]) {
                id[i] = id[id[i]]; // 路径上每个节点都指向祖父节点
                i = id[i];
            }
            return i;
        }

        public boolean connected(int p, int q) {
            return root(p) == root(q);
        }

        public void union(int p, int q) {
            int proot = root(p);
            int qroot = root(q);
            if (proot == qroot) {
                return;
            }
            if (sz[proot] < sz[qroot]) {
                id[proot] = qroot;
                sz[qroot] += sz[proot];
            } else {
                id[qroot] = proot;
                sz[proot] += sz[qroot];
            }
        }
    }
}
