#ifndef TUODAO_RPC_MSG_H_
#define TUODAO_RPC_MSG_H_

#include <cstdint>
#include <string>
#include <chrono>
#include <memory>
#include <iostream>
// #include <functional>
// #include"tdsocket_global.h"
#define TDSOCKET_EXPORT 

namespace tuodao
{
  namespace rpc {
    
    using MsgSize = std::uint32_t;
    using MsgID = std::uint32_t;
    using MsgType = std::uint64_t;

    enum MSG_TYPE {
      REQUEST = 0,
      RESPONSE = 1,
      NOTIFY = 2,
      PING = 3,
      PONG = 4,
      HEART_BEAT = 5,
    };

    // 兼容性协议头（40字节） 老版本协议兼容，妥协改造版本, 整体消息结构有所浪费
    #pragma pack(push, 1)
    struct MsgHeader {
        MsgHeader(): body_len(0), msg_id(0), msg_type(0), req_id(0), reserved2(0), reserved3(0){}
        uint32_t body_len;      
        uint32_t msg_id;        // 消息ID   
        uint64_t msg_type;      // 消息类型 || 1个字节(1-请求 2-应答 3-心跳 4-状态) || 1个字节(编码协议:1-JSON字符串 2-Proto 3-msspack 4-其他) || 6个字节保留 
        uint64_t req_id;        // 保留字段 || 8个字节(报生成毫秒时间戳, 例: 1698468235000, 既是包生成时间，低并发下也可以作为请求id)
        uint64_t reserved2;     
        uint64_t reserved3;     // 保留字段 || 高位：6个字节保留 || 2个字节(包头有效性标志位:0xAA55)
    };
    #pragma pack(pop)

    class TDSOCKET_EXPORT MsgBase
    {
    public:
      auto virtual resize(MsgSize size)->int32_t = 0;
      auto virtual header()->MsgHeader& = 0;
      auto virtual header()const->const MsgHeader& = 0;
      auto virtual capacity()const->MsgSize = 0;
      auto empty()const->bool { return size() == 0; }
      auto size() const->MsgSize { return header().body_len; }
      auto setType(MsgType msg_type)->void { header().msg_type = msg_type; }
      auto type() const->MsgType { return header().msg_type; }
      auto setMsgID(MsgID msg_id)->void { header().msg_id = msg_id; }
      auto msgID() const->MsgID { return header().msg_id; }
      auto data() const->const char* { return const_cast<MsgBase*>(this)->data(); }
      auto data()->char* {
        return reinterpret_cast<char*>(&header()) + sizeof(MsgHeader); }
      auto copy(const std::string &str)->void;
      auto copy(const void *src, MsgSize size)->void;
      auto copyAt(const void *src, MsgSize size, MsgSize at_this_pos_of_msg)->void;
      auto copyMore(const void *src, MsgSize size)->void;
      template<class... Args>
      auto copyStruct(const Args&... args)->void
      {
        resize(0);
        copyStructMore(args...);
      }
      template<class FirstArg, class... Args>
      auto copyStructMore(const FirstArg& first_arg, const Args&... args)->void
      {
        copyMore(static_cast<const void*>(&first_arg), sizeof(FirstArg));
        copyStructMore(args...);
      }
      auto copyStructMore()->void {};
      auto paste(void *tar, MsgSize size) const->void;
      auto paste(void *tar) const->void;
      auto pasteAt(void *tar, MsgSize size, MsgSize at_this_pos_of_msg) const->void;
      template<class FirstArg, class... Args>
      auto pasteStruct(FirstArg& first_arg, Args&... args) const->void
      {
        pasteAt(static_cast<void*>(&first_arg), sizeof(FirstArg), paste_id_);
        paste_id_ += sizeof(FirstArg);
        pasteStruct(args...);
      }
      auto pasteStruct() const->void { paste_id_ = 0; }

      auto toString()const->std::string { return std::string(data(), size()); }

    protected:
      virtual ~MsgBase() = default;
      MsgBase() = default;
      MsgBase(const MsgBase &other) = default;
      MsgBase(MsgBase &&other) = default;
      MsgBase& operator=(const MsgBase& other) = default;
      MsgBase& operator=(MsgBase&& other) = default;

    private:
      mutable MsgSize paste_id_{ 0 };
    };
    
    class TDSOCKET_EXPORT Msg final :public MsgBase
    {
    public:
      auto virtual resize(MsgSize size)->int32_t override;
      auto virtual header()->MsgHeader& override;
      auto virtual header()const->const MsgHeader& override;
      auto virtual capacity()const->MsgSize override { return capacity_; }
      auto swap(Msg &other)->void;

      virtual ~Msg();
      explicit Msg(MsgID msg_id = 0, MsgSize size = 0);
      explicit Msg(const std::string &msg_str);
      Msg(const MsgBase &other);
      Msg(const Msg& other);
      Msg(Msg&& other);
      Msg& operator=(Msg &&other);

    private:
      std::unique_ptr<char[]> data_;
      MsgSize capacity_;
    };
    template<std::size_t CAPACITY>
    class MsgFix final :public MsgBase
    {
    public:
      auto virtual resize(MsgSize size)->int32_t override { header().body_len = size; return 0;};
      auto virtual header()->MsgHeader& override { return *reinterpret_cast<MsgHeader*>(data_); };
      auto virtual header()const->const MsgHeader& override { return *reinterpret_cast<const MsgHeader*>(data_); };
      auto virtual capacity()const->MsgSize override { return CAPACITY; }

      virtual ~MsgFix() = default;
      explicit MsgFix(MsgID msg_id = 0, MsgSize size = 0) :MsgBase() { resize(size); setMsgID(msg_id); }
      MsgFix(const MsgBase &other)
      {
        resize(other.size());
        std::copy_n(reinterpret_cast<const char*>(&other.header()), other.size() + sizeof(MsgHeader), reinterpret_cast<char*>(&header()));
      }
      MsgFix(const MsgFix &other) = default;
      MsgFix(MsgFix &&other) = default;
      MsgFix &operator=(const MsgFix& other) = default;
      MsgFix &operator=(MsgFix&& other) = default;

    private:
      char data_[CAPACITY + sizeof(MsgHeader)];
    };
    
    class MsgStreamBuf :public std::streambuf
    {
    public:
      explicit MsgStreamBuf(MsgBase& msg);
      auto reset()->void;
    protected:
      virtual auto overflow(int_type c)->int_type override;
      virtual auto sync()->int override;

    private:
      MsgBase * msg_;
    };

    class MsgStream : public std::iostream
    {
    public:
      auto reset()->void { buf.reset(); }
      explicit MsgStream(MsgBase& msg) : std::iostream(&buf), buf(msg) { }

    private:
      MsgStreamBuf buf;
    };

    template<typename Function, typename ...Args>
    auto benchmark(std::size_t count, Function func, Args&&... args)->double
    {
      auto begin_time = std::chrono::high_resolution_clock::now();
      for (std::size_t i = 0; i < count; ++i)func(std::forward<Args>(args)...);
      auto end_time = std::chrono::high_resolution_clock::now();

      return double((end_time - begin_time).count()) / 1e9 / count;
    }
  }
}
#endif
