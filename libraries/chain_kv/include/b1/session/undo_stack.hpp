#pragma once

#include <vector>

#include <session/session.hpp>

namespace eosio::session {

   template <typename Session>
   class undo_stack {
    public:
      undo_stack(Session&& head) : m_sessions(std::move(head)) {}
      undo_stack(const undo_stack&) = default;
      undo_stack(undo_stack&&)      = default;

      undo_stack& operator=(const undo_stack&) = default;
      undo_stack& operator=(undo_stack&&) = default;

      void push() { sessions.emplace_back(Session{sessions.back()}); }
      void squash() {
         if ( !sessions.empty() && sessions.back() ) {
            sessions.back().commit();
            sessions.pop_back();
         }
      }
      void undo() {
         if ( !sessions.empty() && sessions.back() ) {
            sessions.back().undo();
            sessions.pop_back();
         }
      }
      void commit(bool from_front=false) {
         if ( !sessions.empty() && sessions.back() ) {
            if (from_front) {
               sessions.front().commit();
            } else {
               sessions.back().commit();
               sessions.pop_back();
            }
         } else if ( !sessions.empty() && !sessions.back() ) {
            sessions.clear();
         }
      }

      bool   empty() const { return sessions.empty(); }
      size_t size() const  { return sessions.size(); }

      Session&       top() { return sessions.back(); }
      const Session& top() const { return sessions.back(); }

    private:
      std::vector<uint64_t> nums;
      std::vector<Session> m_sessions;
   };

} // namespace eosio::session
