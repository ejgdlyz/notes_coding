# Algorithms on Coursea - Princeton
记录 Coursera 的 Algorithm 的笔记和作业实现。

## Introduce
Java 和 edu.princeton.cs.algs4 库的基本使用案例。

## Week 1
## [union-find](./1_union-find/UnionFind.java)
1. 动态连接（dynamic connectivity）
    - union: 连接两个对象。
    - find/connected query: 两个对象是否存在路径连接或是否处于同一连通分量（connected components）中。

2. 快速查找（quick find）
    - data structure
        - 大小为 N 的数组 id[].
    - find: p 和 q 是否连通：即具有相同的 id。
    - union: 合并 p 和 q，即将数组中所有等于 id[q] 的项设置为 id[p]（不适合大量数据）。
3. 快速联合（quick union）
    - data structure: 
        - 大小为 N 的数组 id[]。
        - 将数组 id[] 看作一组数或者森林，id[i] 表示 i 的父节点。
        - id 的根为 id[id[id[...id[i]...]]]
    - find: 检查两个节点的根是否相同。 
    - union: 合并 p 和 q 所在的连通分量，即 p 的根节点对应的 id 值设置为 q 节点对象的 id 值。
4. quick-union improvements
    - WeightedUnionFind
        - improvements
            - 修改 quick-union 避免高树。
            - 记录每棵树的大小。
            - 将小树的根节点作为大树的根节点以维持平衡。
        - Data structure: 
            - id[]: 同 quick-union
            - sz[]: 统计根节点 i 中的节点数
        - find: 同 quick-union, 即 root(p) == root(q)。
        - union: 
            - 链接小树根到大树根
            - 更新 sz 数组
        - union 和 find 的时间复杂度为 Olog(n)。
    - path compression
        - improvements
            - 当查找 p 的根节点时，需要遍历从该节点 p 到根节点的每个节点。
            - 将此路径上的每个节点都指向根节点，这只需要常数的代价。
        - implementation
            - 在 root() 的循环中设置每个节点到根节点的 id
            - 路径上每个节点都指向其祖父节点 id[i] = id[id[i]]。尽管此操作不如完全展平好，但是实际性能差别不大。
    - implementation: 见 [UnionFind](./1_union-find/UnionFind.java)
5. Programming Homework
    - homework:
        - [Percolation](./1_union-find/Percolation.java) 
        - [PercolationStatus](./1_union-find/PercolationStats.java)。
    - interview problems:
        - Social network 见 [UnonFind](./1_union-find/UnionFind.java) 的 PathCompressionWeightedQuickUnionUF。
        
## Analysis of Algorithms
