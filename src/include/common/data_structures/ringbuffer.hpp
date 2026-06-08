#include <array>
#include <cstddef>
#include <initializer_list>
#include <mutex>
#include <optional>

template <typename ValueType, std::size_t Capacity>
class RingBuffer {
  public:

      RingBuffer() : _data{}, _readIndex(0), _writeIndex(0),_size(0) {
          
      }

      RingBuffer(std::initializer_list<ValueType> data) : RingBuffer{ } {
          for (const ValueType& element : data) {
              push(element);
          }
      }
      
      void push(ValueType value) {
          std::lock_guard<std::mutex> lck {_mtx};

          _data[_writeIndex] = value;
          _writeIndex = (_writeIndex + 1) % Capacity;
          
          if (_size == Capacity) {
              _readIndex = (_readIndex + 1) % Capacity;
          } else {
            _size++;
          }
      }
      
      std::optional<ValueType> pop() {
          std::lock_guard<std::mutex> lck {_mtx};

          if (_size == 0) return std::nullopt;
          
          ValueType output { _data[_readIndex] };
          _readIndex = (_readIndex + 1) % Capacity;
          _size--;

          return output;
      }
    
  private:
        
    std::array<ValueType, Capacity> _data;
    std::size_t _readIndex;
    std::size_t _writeIndex;
    std::size_t _size;

     std::mutex _mtx;
};