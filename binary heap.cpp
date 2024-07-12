#include <algorithm>
#include <iostream>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

// binary heap with the comparison-winning element on top, by default the smallest one
template <typename T, typename Comparator = std::less<T>>
class binary_heap {
public:

    binary_heap(const std::vector<T>& raw_data = {}) {
        data = raw_data;
        heapify();
    }

    friend std::ostream& operator<<(std::ostream& out, const binary_heap& heap) {
        for (size_t i = 0; i < heap.data.size(); i++) {
            out << heap.data[i] << " ";
            // if i + 2 is power of 2, or i is the last one, then the layer is completed
            if (!((i + 1) & (i + 2)) || i == heap.data.size() - 1) {
                out << "\n";
            }
        }
        return out;
    }

    void push(T element) {
        data.push_back(element);
        sift_up(data.size() - 1);
    }

    T top() {
        return data[0];
    }

    T pop() {
        T top = data[0];
        std::swap(data[0], data[data.size() - 1]);
        data.pop_back();
        down_heapify(0);
        return top;
    }

private:

    std::vector<T> data;

    std::optional<size_t> calculate_parent(size_t index) {
        return (index > 0) ? std::make_optional((index - 1) / 2) : std::nullopt;
    }

    std::pair<std::optional<size_t>, std::optional<size_t>> calculate_children(size_t index) {
        return std::make_pair(
            (2*index + 1 < data.size()) ? std::make_optional(2*index + 1) : std::nullopt,
            (2*index + 2 < data.size()) ? std::make_optional(2*index + 2) : std::nullopt);
    }

    void heapify() {
        for (std::make_signed_t<size_t> i = data.size() / 2; i >= 0; i--) {
            down_heapify(i);
        }
    }

    void down_heapify(size_t index) {
        // both children exist
        if (calculate_children(index).second.has_value()) {
            // compare children, then compare with comparison winning one, swap and down heapify if necessary
            size_t comparison_winning_child = (Comparator()(data[calculate_children(index).first.value()], data[calculate_children(index).second.value()])) 
                ? calculate_children(index).first.value() : calculate_children(index).second.value();
            if (Comparator()(data[comparison_winning_child], data[index])) {
                std::swap(data[comparison_winning_child], data[index]);
                down_heapify(comparison_winning_child);
            }
        } else {
            // one child exists
            if (calculate_children(index).first.has_value()) {
                // compare with child, swap and down heapify if necessary
                if (Comparator()(data[calculate_children(index).first.value()], data[index])) {
                    std::swap(data[calculate_children(index).first.value()], data[index]);
                    down_heapify(calculate_children(index).first.value());
                }
            }
        }
    }

    void sift_up(size_t index) {
        if (calculate_parent(index).has_value()) {
            if (Comparator()(data[index], data[calculate_parent(index).value()])) {
                std::swap(data[calculate_parent(index).value()], data[index]);
                sift_up(calculate_parent(index).value());
            }
        }
    }
};

int main() {

    binary_heap<int> heap;

    bool flag = false;
    while (!flag) {
        std::string command;
        std::cin >> command;
        if (command == "push") {
            int element;
            std::cin >> element;
            heap.push(element);
        } else if (command == "pop") {
            std::cout << heap.pop() << "\n";
        } else if (command == "top") {
            std::cout << heap.top() << "\n";
        } else if (command == "end") {
            flag = true;
        } else if (command == "show") {
            std::cout << heap;
        }
    }

}