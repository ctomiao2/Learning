#include <iostream>
#include <vector>
#include <cstring>

using namespace std;

class MaximalSquare221 {
    public:
        int maximalSquare(vector<vector<char>>& matrix) {
            if (matrix.size() == 0 || matrix[0].size() == 0)
                return 0;
            int m = matrix.size(), n = matrix[0].size();
            int **maxLengthLeft, **maxLengthTop;
            int *maxSquare = (int*) malloc(n * sizeof(int));
            memset(maxSquare, 0, n * sizeof(int));
            maxLengthLeft = (int**) malloc(m * sizeof(int*));
            maxLengthTop = (int**) malloc(m * sizeof(int*));
            for (int i = 0; i < m; ++i) {
                maxLengthLeft[i] = (int*) malloc(n * sizeof(int));
                maxLengthTop[i] = (int*) malloc(n * sizeof(int));
            }
            
            int maxL = 0;
            int curMaxL = 0;
            for (size_t r = 0; r < matrix.size(); ++r) {
                for (size_t c = 0; c < matrix[r].size(); ++c) {
                    if (c > 0)
                        maxLengthLeft[r][c] = matrix[r][c] == '0' ? 0 : maxLengthLeft[r][c-1] + 1;
                    else
                        maxLengthLeft[r][c] = matrix[r][c] == '0' ? 0 : 1;
                    
                    if (r > 0)
                        maxLengthTop[r][c] = matrix[r][c] == '0' ? 0 : maxLengthTop[r-1][c] + 1;
                    else
                        maxLengthTop[r][c] = matrix[r][c] == '0' ? 0 : 1;
                    
                    int a = maxLengthLeft[r][c] > maxLengthTop[r][c] ? maxLengthTop[r][c] : maxLengthLeft[r][c];
                    if (c > 0) {
                        a = maxSquare[c-1] < a-1 ? maxSquare[c-1]+1 : a;
                        maxSquare[c-1] = curMaxL;
                    }

                    curMaxL = 0;
                    for (int x = a; x > 0; --x) {
                        if (maxLengthLeft[r-x+1][c] >= x && maxLengthTop[r][c-x+1] >= x) {
                            curMaxL = x;
                            if (curMaxL > maxL)
                                maxL = curMaxL;
                            break;
                        }
                    }
                    
                }
            }
            
            return maxL * maxL;
        }
};

struct TreeNode {
    int val;
    TreeNode *left;
    TreeNode *right;
    TreeNode(int x): val(x), left(NULL), right(NULL) {}
};

class Solution222{
public:
    int countNodes(TreeNode* root) {
        int ret = 0;
        visit(root, ret);
        return ret;
    }

    void visit(TreeNode* root, int &count) {
        if (root == NULL)
            return;
        ++count;
        visit(root->left);
        visit(root->right);
    }
};

int main() {
    Solution222 solution;
    vector<vector<char>> v;
    char r1[] = {'1', '0', '1', '1', '0', '1'};
    char r2[] = {'1', '1', '1', '1', '1', '1'};
    char r3[] = {'0', '1', '1', '0', '1', '1'};
    char r4[] = {'1', '1', '1', '0', '1', '0'};
    char r5[] = {'0', '1', '1', '1', '1', '1'};
    char r6[] = {'1', '1', '0', '1', '1', '1'};
    int n = 5;
    //v.push_back(vector<char>(r1, r1+n));
    //v.push_back(vector<char>(r2, r2 + n));
    v.push_back(vector<char>(r3, r3+2));
    //v.push_back(vector<char>(r4, r4 + n));
    //v.push_back(vector<char>(r5, r5 + n));
    //v.push_back(vector<char>(r6, r6 + n));
    cout << "ret: " << solution.maximalSquare(v) << endl;
}
