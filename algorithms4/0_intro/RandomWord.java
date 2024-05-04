import edu.princeton.cs.algs4.StdIn;
import edu.princeton.cs.algs4.StdOut;
import edu.princeton.cs.algs4.StdRandom;

// G:\NOTES\Algorithms\algs4.jar
public class RandomWord {
    public static void main(String[] args) {
        String champion = null;
        String tmp = null;
        double cnt = 1;
        while (!StdIn.isEmpty()) {
            tmp = StdIn.readString();
            if (StdRandom.bernoulli(1 / cnt)) {
                champion = tmp;
            }
            ++cnt;
        }
        StdOut.println(champion);
    }

}
