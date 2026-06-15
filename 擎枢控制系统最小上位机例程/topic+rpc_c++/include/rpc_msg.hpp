/*
 * @Author: garin.yang
 * @Date: 2024-09-03 16:47:08
 * @LastEditors: yangbo
 * @LastEditTime: 2024-09-14 14:00:52
 * @Description: 
 * @FilePath: .../include/rpc_msg.hpp  (RPC / cpp_rpc 使用，namespace core)
 */
#ifndef CODEIT_CORE_MSG_RPC_H_
#define CODEIT_CORE_MSG_RPC_H_

#include <cstdint>
#include <string>
#include <chrono>
#include <memory>
#include <iostream>
#include "util.hpp"

namespace core
{
using MsgSize = std::uint32_t;
using MsgID = std::uint32_t;
using MsgType = std::uint64_t;

/* 仅添加注释，并未修改！！！ by garin.yang 2024-01-24 */
struct MsgHeader
{
  MsgHeader(): msg_size_(0), msg_id_(0), msg_type_(1), reserved1_(GetCurrentTS()), reserved2_(GetCurrentTS()), reserved3_(202401){
  }
                            /* 协议体改造优化梳理 2024.01.17 */
  MsgSize msg_size_;        /* 消息体大小 */
  MsgID msg_id_;            /* 消息ID (用于区分业务类型，但是没法匹配 "请求/应答")，用于 [监控中心] 统计业务场景 */ 
  MsgType msg_type_;        /* 1-需要应答的请求/2-应答/3-无需应答的命令/其他 */
  std::int64_t reserved1_;  /* 请求id：单调自增id（可用时间戳，考虑时间对齐影响）, 相同msg_id下的请求id不重复,用于关联"请求/应答"，
                              相同seq_id一一对应，用于 [监控中心] 跟踪包是否请求应答有无丢失 */
  std::int64_t reserved2_;  /* 包生成时间戳（微秒）：1705482255000 用于 [监控中心] 跟踪响应耗时 */
  std::int64_t reserved3_;  /* 预留版本信息 */
};

class CPP_RPC_EXPORT MsgBase {
public:
  auto virtual resize(MsgSize size)->void = 0;
  auto virtual header()->MsgHeader& = 0;
  auto virtual header()const->const MsgHeader& = 0;
  auto virtual capacity()const->MsgSize = 0;
  auto empty()const->bool { return size() == 0; }
  auto size() const->MsgSize { return header().msg_size_; }
  auto setType(MsgType msg_type)->void { header().msg_type_ = msg_type; }
  auto type() const->MsgType { return header().msg_type_; }
  auto setMsgID(MsgID msg_id)->void { header().msg_id_ = msg_id; }
  /* [BGN] added by garin.yang 2024-01-24 */
  auto setMsgSeqID(const int64_t& seq_id)->void { header().reserved1_ = seq_id;}      // 请求id ms
  auto seqID() const->int64_t { return header().reserved1_;}                          // 请求id ms
  auto setMsgBornTime(const int64_t& born_ts)->void { header().reserved2_ = born_ts;} // 包出生时间 ms
  /* [END] */
  auto msgID() const->MsgID { return header().msg_id_; }
  auto data() const->const char* { return const_cast<MsgBase*>(this)->data(); }
  auto data()->char* {
    // auto s = reinterpret_cast<char*>(&header());
    // auto s1= reinterpret_cast<char*>(&header()) + sizeof(MsgHeader);
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

class CPP_RPC_EXPORT Msg final :public MsgBase
{
public:
  auto virtual resize(MsgSize size)->void override;
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
  auto virtual resize(MsgSize size)->void override { header().msg_size_ = size; };
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
  //explicit MsgStream(MsgBase& msg) : buf(msg), std::iostream(&buf) { }
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
};
}

#endif

// // 自定义消息结构
// #include <iostream>

// #ifndef __COMMUNICATOR_MSG__
// #define __COMMUNICATOR_MSG__

// struct MsgHead
// {
//     uint64_t msg_id;
// };

// struct Msg {
//     std::string header;
//     std::string body;

//     Msg(const std::string& header, const std::string& body)
//         : header(header), body(body) {}

//     // 用于输出调试信息 
//     friend std::ostream& operator<<(std::ostream& os, const Msg& msg) {
//         os << "Header: " << msg.header << ", Body: " << msg.body;
//         return os;
//     }
// };
// #endif // !__COMMUNICATOR_MSG__