#include <iostream>
#include <cstdlib>
#include <string>

// 定义二叉树节点结构
struct Node
{
    int value;
    Node *left;
    Node *right;
};

// 构建二叉树
Node *createTree()
{
    // 创建所有节点
    Node *n1 = new Node{36, nullptr, nullptr};
    Node *n21 = new Node{8, nullptr, nullptr};
    Node *n22 = new Node{50, nullptr, nullptr};
    Node *n31 = new Node{6, nullptr, nullptr};
    Node *n32 = new Node{22, nullptr, nullptr};
    Node *n33 = new Node{45, nullptr, nullptr};
    Node *n34 = new Node{107, nullptr, nullptr};
    Node *n41 = new Node{1, nullptr, nullptr};
    Node *n42 = new Node{7, nullptr, nullptr};
    Node *n43 = new Node{20, nullptr, nullptr};
    Node *n44 = new Node{35, nullptr, nullptr};
    Node *n45 = new Node{40, nullptr, nullptr};
    Node *n46 = new Node{47, nullptr, nullptr};
    Node *n47 = new Node{99, nullptr, nullptr};
    Node *n48 = new Node{1001, nullptr, nullptr};

    // 连接节点
    n1->left = n21;
    n1->right = n22;
    n21->left = n31;
    n21->right = n32;
    n22->left = n33;
    n22->right = n34;
    n31->left = n41;
    n31->right = n42;
    n32->left = n43;
    n32->right = n44;
    n33->left = n45;
    n33->right = n46;
    n34->left = n47;
    n34->right = n48;

    return n1; // 返回根节点
}

// 递归函数 fun7
int fun7(Node *node, int target)
{
    if (node == nullptr)
    {
        return -1;
    }

    if (node->value == target)
    {
        return 0;
    }
    else if (target < node->value)
    {
        return 2 * fun7(node->left, target);
    }
    else
    {
        return 2 * fun7(node->right, target) + 1;
    }
}

// 读取输入行
std::string read_line()
{
    std::string input;
    std::getline(std::cin, input);
    return input;
}

// 炸弹爆炸函数
void explode_bomb()
{
    std::cerr << "Bomb exploded!" << std::endl;
}

// secret_phase 函数
void secret_phase(Node *root)
{
    for (int i = 0; i < 1002; i++)

    {
        int result = fun7(root, i);

        if (result == 7)
        {
            std::cout << i << ' ' << "Phase defused!" << std::endl;
        }
    }
}

int main()
{
    Node *root = createTree();
    secret_phase(root);

    // 清理内存
    // 这里省略了删除节点的代码，实际应用中应当释放所有动态分配的内存
    return 0;
}