#pragma once

#include <vector>

#include <session/session.hpp>

namespace eosio::session {

template <typename Session>
class undo_stack;

template <typename Data_store>
undo_stack<session<Data_store>> make_undo_stack(Data_store data_store);

template <typename Session>
class undo_stack final {
 public:
   undo_stack(Session head);
   undo_stack(const undo_stack&) = default;
   undo_stack(undo_stack&&)      = default;

   undo_stack& operator=(const undo_stack&) = default;
   undo_stack& operator=(undo_stack&&) = default;

   void push();
   void squash();
   void undo();
   void commit(int64_t revision);

   bool   empty() const;
   size_t size() const;

   int64_t revision() const;
   void    revision(int64_t revision);

   Session&       top();
   const Session& top() const;

 private:
   int64_t              m_revision{ 0 };
   std::vector<Session> m_sessions;
};

template <typename Data_store>
undo_stack<session<Data_store>> make_undo_stack(Data_store data_store) {
   return { std::move(data_store) };
}

template <typename Session>
undo_stack<Session>::undo_stack(Session head_session) : m_sessions{ 1, std::move(head_session) } {}

template <typename Session>
void undo_stack<Session>::push() {
   m_sessions.emplace_back(make_session(m_sessions.front()));
   ++m_revision;
}

template <typename Session>
void undo_stack<Session>::squash() {
   if (m_sessions.size() < 3) {
      return;
   }

   m_sessions.front().detach();
   auto from_session = m_sessions.back();
   m_sessions.pop_back();

   m_sessions.front().attach(m_sessions.back());
   m_sessions.back().attach(from_session);
   from_session.commit();
   m_sessions.back().detach();
   --m_revision;
}

template <typename Session>
void undo_stack<Session>::undo() {
   if (m_sessions.size() < 2) {
      // The session as the front can't be popped.
      return;
   }
   m_sessions.front().detach();
   m_sessions.pop_back();
   --m_revision;

   if (m_sessions.size() > 1) {
      m_sessions.front().attach(m_sessions.back());
   }
}

template <typename Session>
void undo_stack<Session>::commit(int64_t revision) {
   revision              = std::min(revision, m_revision);
   auto initial_revision = m_revision - m_sessions.size() + 1;
   if (initial_revision > revision) {
      return;
   }

   m_sessions.front().detach();

   auto start_index       = revision - initial_revision;
   auto session_to_commit = m_sessions[start_index++];
   auto current_index     = 1;
   for (size_t i = start_index; i < m_sessions.size(); ++i) { m_sessions[current_index++] = m_sessions[start_index]; }
   m_sessions.erase(std::begin(m_sessions) + current_index, std::end(m_sessions));

   m_sessions.front().attach(session_to_commit);
   session_to_commit.commit();
   m_sessions.front().detach();

   if (m_sessions.size() > 1) {
      m_sessions.front().attach(m_sessions.back());
   }
}

template <typename Session>
bool undo_stack<Session>::empty() const {
   return m_sessions.size() < 2;
}

template <typename Session>
size_t undo_stack<Session>::size() const {
   return m_sessions.size() - 1;
}

template <typename Session>
int64_t undo_stack<Session>::revision() const {
   return m_revision;
}

template <typename Session>
void undo_stack<Session>::revision(int64_t revision) {
   if (!empty()) {
      return;
   }

   if (revision <= m_revision) {
      return;
   }

   m_revision = revision;
}

template <typename Session>
Session& undo_stack<Session>::top() {
   return m_sessions.back();
}

template <typename Session>
const Session& undo_stack<Session>::top() const {
   return m_sessions.back();
}

} // namespace eosio::session
