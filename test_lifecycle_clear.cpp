// 测试：为什么 on_activate 和 on_deactivate 都需要 clear()

#include <iostream>
#include <vector>
#include <memory>

class Interface {
public:
    Interface(int id) : id_(id) {
        std::cout << "  Interface " << id_ << " created" << std::endl;
    }
    ~Interface() {
        std::cout << "  Interface " << id_ << " destroyed" << std::endl;
    }
private:
    int id_;
};

// ❌ 错误示例：只在 deactivate 中 clear
class ControllerWrong {
private:
    std::vector<std::unique_ptr<Interface>> interfaces_;
    
public:
    void on_activate() {
        std::cout << "on_activate() - size before: " << interfaces_.size() << std::endl;
        
        // ❌ 没有 clear()
        
        for (int i = 1; i <= 4; ++i) {
            interfaces_.push_back(std::make_unique<Interface>(i));
        }
        
        std::cout << "on_activate() - size after: " << interfaces_.size() << std::endl;
    }
    
    void on_deactivate() {
        std::cout << "on_deactivate() - clearing" << std::endl;
        interfaces_.clear();
    }
    
    void on_error() {
        std::cout << "on_error() - NOT clearing (simulating error)" << std::endl;
        // ❌ 错误处理时没有 clear
    }
};

// ✅ 正确示例：两个地方都 clear
class ControllerCorrect {
private:
    std::vector<std::unique_ptr<Interface>> interfaces_;
    
public:
    void on_activate() {
        std::cout << "on_activate() - size before: " << interfaces_.size() << std::endl;
        
        // ✅ 先 clear()
        interfaces_.clear();
        
        for (int i = 1; i <= 4; ++i) {
            interfaces_.push_back(std::make_unique<Interface>(i));
        }
        
        std::cout << "on_activate() - size after: " << interfaces_.size() << std::endl;
    }
    
    void on_deactivate() {
        std::cout << "on_deactivate() - clearing" << std::endl;
        interfaces_.clear();
    }
    
    void on_error() {
        std::cout << "on_error() - NOT clearing (simulating error)" << std::endl;
        // 错误处理时没有 clear，但 on_activate 会处理
    }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "测试 1: 正常流程（经过 deactivate）" << std::endl;
    std::cout << "========================================" << std::endl;
    
    {
        std::cout << "\n--- 错误示例 ---" << std::endl;
        ControllerWrong wrong;
        wrong.on_activate();
        wrong.on_deactivate();
        wrong.on_activate();  // 正常，因为经过了 deactivate
    }
    
    {
        std::cout << "\n--- 正确示例 ---" << std::endl;
        ControllerCorrect correct;
        correct.on_activate();
        correct.on_deactivate();
        correct.on_activate();  // 正常
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "测试 2: 异常流程（跳过 deactivate）" << std::endl;
    std::cout << "========================================" << std::endl;
    
    {
        std::cout << "\n--- 错误示例 ---" << std::endl;
        ControllerWrong wrong;
        wrong.on_activate();
        wrong.on_error();      // 跳过 deactivate
        wrong.on_activate();   // ❌ 问题：会重复添加
    }
    
    {
        std::cout << "\n--- 正确示例 ---" << std::endl;
        ControllerCorrect correct;
        correct.on_activate();
        correct.on_error();    // 跳过 deactivate
        correct.on_activate(); // ✅ 正常：on_activate 会 clear
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "测试 3: 多次错误恢复" << std::endl;
    std::cout << "========================================" << std::endl;
    
    {
        std::cout << "\n--- 错误示例 ---" << std::endl;
        ControllerWrong wrong;
        wrong.on_activate();
        wrong.on_error();
        wrong.on_activate();   // size = 8
        wrong.on_error();
        wrong.on_activate();   // size = 12 ❌
    }
    
    {
        std::cout << "\n--- 正确示例 ---" << std::endl;
        ControllerCorrect correct;
        correct.on_activate();
        correct.on_error();
        correct.on_activate(); // size = 4 ✅
        correct.on_error();
        correct.on_activate(); // size = 4 ✅
    }
    
    return 0;
}
