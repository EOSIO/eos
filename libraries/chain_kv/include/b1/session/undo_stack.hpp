#pragma once

#include <queue>

#include <session/session.hpp>

namespace eosio::session {

template <typename Session>
class undo_stack final {
 public:
   using session_type = session<Session>;

   undo_stack(Session& head);
   undo_stack(const undo_stack&) = delete;
   undo_stack(undo_stack&&)      = default;

   undo_stack& operator=(const undo_stack&) = delete;
   undo_stack& operator=(undo_stack&&) = default;

   void push();
   void squash();
   void undo();
   void commit(int64_t revision);

   bool   empty() const;
   size_t size() const;

   int64_t revision() const;
   void    revision(int64_t revision);

   session_type&       top();
   const session_type& top() const;

 private:
   int64_t                  m_revision{ 0 };
   Session*                 m_head;
   std::deque<session_type> m_sessions; // Need a deque so pointers don't become invalidated.  The session holds a
                                        // pointer to the parent internally.
};

template <typename Session>
undo_stack<Session>::undo_stack(Session& head) : m_head{ &head } {
   m_sessions.emplace_back(*m_head);
}

template <typename Session>
void undo_stack<Session>::push() {
   if (m_sessions.empty()) {
      m_sessions.emplace_back(*m_head);
   } else {
      m_sessions.emplace_back(m_sessions.back());
   }
   ++m_revision;
}

template <typename Session>
void undo_stack<Session>::squash() {
   if (m_sessions.empty()) {
      return;
   }
   m_sessions.back().commit();
   m_sessions.back().detach();
   m_sessions.pop_back();
   --m_revision;
}

template <typename Session>
void undo_stack<Session>::undo() {
   if (m_sessions.empty()) {
      return;
   }
   m_sessions.back().detach();
   m_sessions.pop_back();
   --m_revision;
}

template <typename Session>
void undo_stack<Session>::commit(int64_t revision) {
   revision              = std::min(revision, m_revision);
   auto initial_revision = static_cast<int64_t>(m_revision - m_sessions.size() + 1);
   if (initial_revision > revision) {
      return;
   }

   auto  start_index       = revision - initial_revision;
   auto& session_to_commit = m_sessions[start_index++];
   auto  current_index     = 0;
   auto  parent            = m_sessions[0].parent();

   std::visit([&](auto* parent) { session_to_commit.attach(*parent); }, parent);
   session_to_commit.commit();
   session_to_commit.detach();

   for (int64_t i = 0; i <= start_index; ++i) { m_sessions[i].detach(); }
   for (int64_t i = start_index; i < static_cast<int64_t>(m_sessions.size()); ++i) {
      m_sessions[current_index++] = std::move(m_sessions[i]);
   }
   m_sessions.erase(std::begin(m_sessions) + current_index, std::end(m_sessions));
   if (!m_sessions.empty()) {
      std::visit([&](auto* parent) { m_sessions[0].attach(*parent); }, parent);
   }
}

template <typename Session>
bool undo_stack<Session>::empty() const {
   return m_sessions.empty();
}

template <typename Session>
size_t undo_stack<Session>::size() const {
   return m_sessions.size();
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
typename undo_stack<Session>::session_type& undo_stack<Session>::top() {
   return m_sessions.back();
}

template <typename Session>
const typename undo_stack<Session>::session_type& undo_stack<Session>::top() const {
   return m_sessions.back();
}

} // namespace eosio::session
