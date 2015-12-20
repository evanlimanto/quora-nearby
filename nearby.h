/*
 * Copyright 2015 Evan Limanto
 * Solution to Quora's Nearby challenge (https://hackerrank.com/contests/cs-quora/challenges/quora-nearby).
 */

#ifndef _NEARBY_H
#define _NEARBY_H

#include <vector>

namespace NearbySolver {

using std::istream;
using std::vector;

class Topic;
class Question;
class Node;
class KDTree;

constexpr double EPSILON = 1e-3;
constexpr bool compareDouble(const double &a, const double &b) {
  return (a - b) > EPSILON;
}

class Topic {
 public:
  Topic() = default;
  ~Topic() = default;
  double getX() const { return coordinates[0]; }
  double getY() const { return coordinates[1]; }
  double coordinateAt(int dimension) const { return coordinates[dimension]; }
  int getId() const { return id; }
  vector<int>& getQuestionIds() { return questionIds; }

  friend bool operator< (const Topic &topic1, const Topic &topic2);
  friend istream& operator>> (istream &in, Topic &topic);

 private:
  int id = 0;
  vector<int> questionIds;
  vector<double> coordinates = {0.0, 0.0};
};

class Question {
 public:
  Question() = default;
  ~Question() = default;
  int getId() const { return id; }
  friend bool operator< (const Question &question1, const Question &question2);
  friend istream& operator>> (istream &in, Question &question);

 private:
  int id = 0;
  int topicCount = 0;
};

class Node {
 public:
  Node() = default;
  ~Node() = default;
  explicit Node(const Topic &next) :
    left(nullptr), right(nullptr), topic(next) {}

 private:
  Topic topic;
  Node *left = nullptr;
  Node *right = nullptr;

  friend class KDTree;
};

class KDTree {
 public:
  KDTree() = default;
  ~KDTree();
  Node* insert(Node *currentNode, int depth, const Topic &topic);
  void kNNTopics(Node *currentNode, int depth,
                 const vector<double> &queryPosition) const;
  void kNNQuestions(Node *currentNode, int depth,
                    const vector<double> &queryPosition) const;
  void insert(const Topic &topic) {
    root = insert(root, false, topic);
  }
  void kNNTopics(const vector<double> &queryPosition) const {
    kNNTopics(root, false, queryPosition);
  }
  void kNNQuestions(const vector<double> &queryPosition) const {
    kNNQuestions(root, false, queryPosition);
  }

 private:
  Node *root = nullptr;
  void freeNodes(Node *currentNode);
};

}  // namespace NearbySolver

#endif  // _NEARBY_H
