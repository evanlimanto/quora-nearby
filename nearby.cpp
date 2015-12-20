/*
 * Copyright 2015 Evan Limanto
 * Solution to Quora's Nearby challenge (https://hackerrank.com/contests/cs-quora/challenges/quora-nearby).
 *
 * First, insert all topics into the KD-Tree.
 * If query type is a topic, a standard k-NN search is done on the query point.
 * If query type is a question, a standard k-NN search is also done on the query point. However, at the same time,
 * everytime a topic is encountered, update all the questions associated with this topic.
 * When there are more than the required number of results in the set, compares topics with the furthest topic
 * from the query point and removes the question associated with this furthest topic if it is a less optimal answer.
 */

#include "./nearby.h"

#include <cmath>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

namespace NearbySolver {

using std::cin;
using std::cout;
using std::set;
using std::unordered_map;
using std::vector;

int numResults;
vector<double> queryPosition = {0.0, 0.0};

set<Topic> topicSet;
set<Question> questionSet;

unordered_map<int, Topic> topics;
unordered_map<int, Question> questions;

// Mapping from question id to topic id associated with it of topic
// closest to the query coordinate.
unordered_map<int, int> closestQuestionTopic;

KDTree kdtree;

void KDTree::freeNodes(Node *currentNode) {
  if (currentNode == nullptr)
    return;

  freeNodes(currentNode->left);
  freeNodes(currentNode->right);
  delete currentNode;
}

KDTree::~KDTree() {
  freeNodes(root);
}

Node* KDTree::insert(Node *currentNode, int depth, const Topic &topic) {
  if (currentNode == nullptr)
    return new Node(topic);

  int depthParity = depth & 1;
  if (topic.coordinateAt(depthParity) <
      currentNode->topic.coordinateAt(depthParity)) {
    currentNode->left = insert(currentNode->left, depth + 1, topic);
  } else {
    currentNode->right = insert(currentNode->right, depth + 1, topic);
  }
  return currentNode;
}

void KDTree::kNNTopics(
    Node *currentNode, int depth, const vector<double> &queryPosition) const {
  if (currentNode == nullptr)
    return;

  int depthParity = depth & 1;
  topicSet.insert(currentNode->topic);

  while (static_cast<int>(topicSet.size()) > numResults)
    topicSet.erase(prev(topicSet.cend()));

  // Select first node to traverse next.
  Node *firstNode = nullptr, *secondNode = nullptr;
  if (queryPosition[depthParity] <
      currentNode->topic.coordinateAt(depthParity)) {
    firstNode = currentNode->left;
    secondNode = currentNode->right;
  } else {
    firstNode = currentNode->right;
    secondNode = currentNode->left;
  }

  kNNTopics(firstNode, depth + 1, queryPosition);

  // Traverse other node if number of results are not enough or there are
  // potentially more optimal answers.
  if (static_cast<int>(topicSet.size()) < numResults) {
    kNNTopics(secondNode, depth + 1, queryPosition);
  } else if (!topicSet.empty()) {
    const auto iter = prev(topicSet.cend());
    double dist1 =
      fabs(queryPosition[depthParity] -
           currentNode->topic.coordinateAt(depthParity));
    double dist2 = hypot(queryPosition[0] - iter->getX(),
                         queryPosition[1] - iter->getY());

    if (dist1 < dist2) {
      kNNTopics(secondNode, depth + 1, queryPosition);
    }
  }
}

void KDTree::kNNQuestions(
    Node *currentNode, int depth, const vector<double> &queryPosition) const {
  if (currentNode == nullptr)
    return;

  int depthParity = depth & 1;
  for (int questionIndex :
       topics[currentNode->topic.getId()].getQuestionIds()) {
    auto iter = closestQuestionTopic.find(questionIndex);
    if (iter == closestQuestionTopic.cend()) {
      closestQuestionTopic[questionIndex] = currentNode->topic.getId();
      questionSet.insert(questions[questionIndex]);
    } else {
      int topicId = iter->second;
      double dist1 =
        hypot(topics[topicId].getX() - queryPosition[0],
              topics[topicId].getY() - queryPosition[1]);
      double dist2 =
        hypot(topics[currentNode->topic.getId()].getX() - queryPosition[0],
              topics[currentNode->topic.getId()].getY() - queryPosition[1]);

      if (compareDouble(dist1, dist2) ||
          (fabs(dist1 - dist2) <= EPSILON &&
           currentNode->topic.getId() > topicId)) {
        questionSet.erase(questions[questionIndex]);
        iter->second = currentNode->topic.getId();
        questionSet.insert(questions[questionIndex]);
      }
    }
  }

  while (static_cast<int>(questionSet.size()) > numResults) {
    const auto iter = prev(questionSet.cend());
    closestQuestionTopic.erase(iter->getId());
    questionSet.erase(iter);
  }

  // Select first node to traverse next.
  Node *firstNode = nullptr, *secondNode = nullptr;
  if (queryPosition[depthParity] <
      currentNode->topic.coordinateAt(depthParity)) {
    firstNode = currentNode->left;
    secondNode = currentNode->right;
  } else {
    firstNode = currentNode->right;
    secondNode = currentNode->left;
  }

  kNNQuestions(firstNode, depth + 1, queryPosition);

  // Traverse other node if number of results are not enough or there are
  // potentially more optimal answers.
  if (static_cast<int>(questionSet.size()) < numResults) {
    kNNQuestions(secondNode, depth + 1, queryPosition);
  } else if (!questionSet.empty()) {
    const auto iter = prev(questionSet.cend());
    int closestTopicId = closestQuestionTopic[iter->getId()];
    double dist1 =
      fabs(queryPosition[depthParity] -
           currentNode->topic.coordinateAt(depthParity));
    double dist2 = hypot(queryPosition[0] - topics[closestTopicId].getX(),
                         queryPosition[1] - topics[closestTopicId].getY());

    if (dist1 < dist2) {
      kNNQuestions(secondNode, depth + 1, queryPosition);
    }
  }
}

istream& operator>> (istream &in, Topic &topic) {
  in >> topic.id;
  in >> topic.coordinates[0];
  in >> topic.coordinates[1];
  return in;
}

istream& operator>> (istream &in, Question &question) {
  int topicId;
  in >> question.id >> question.topicCount;
  for (int i = 0; i < question.topicCount; ++i) {
    in >> topicId;
    topics[topicId].getQuestionIds().push_back(question.getId());
  }
  return in;
}

// Topic compare function for ordering inside a set.
bool operator< (const Topic &topic1, const Topic &topic2) {
  double dist1 = hypot(topic1.getX() - queryPosition[0],
                       topic1.getY() - queryPosition[1]);
  double dist2 = hypot(topic2.getX() - queryPosition[0],
                       topic2.getY() - queryPosition[1]);

  if (compareDouble(dist1, dist2))
    return false;
  else if (compareDouble(dist2, dist1))
    return true;
  return topic1.getId() > topic2.getId();
}

// Question compare function for ordering inside a set.
bool operator< (const Question &question1, const Question &question2) {
  int topicAssociated1 = closestQuestionTopic[question1.getId()];
  int topicAssociated2 = closestQuestionTopic[question2.getId()];

  double dist1 =
    hypot(topics[topicAssociated1].getX() - queryPosition[0],
          topics[topicAssociated1].getY() - queryPosition[1]);
  double dist2 =
    hypot(topics[topicAssociated2].getX() - queryPosition[0],
          topics[topicAssociated2].getY() - queryPosition[1]);

  if (compareDouble(dist1, dist2))
    return false;
  if (compareDouble(dist2, dist1))
    return true;
  return question1.getId() > question2.getId();
}

void inputQuestion() {
  Question currentQuestion;
  cin >> currentQuestion;
  questions[currentQuestion.getId()] = currentQuestion;
}

const Topic& inputTopic() {
  Topic currentTopic;
  cin >> currentTopic;
  return topics[currentTopic.getId()] = currentTopic;
}

template <typename T>
void printSet(const set<T> &itemSet) {
  bool isFirstElem = true;
  for (const auto &iter : itemSet) {
    if (!isFirstElem)
      cout << " ";
    isFirstElem = false;
    cout << iter.getId();
  }
  cout << std::endl;
}

void solve() {
  int T, Q, N;
  char queryType;

  cin >> T >> Q >> N;

  for (int i = 1; i <= T; ++i) {
    const Topic &currentTopic = inputTopic();
    kdtree.insert(currentTopic);
  }

  for (int i = 1; i <= Q; ++i) {
    inputQuestion();
  }

  for (int i = 0; i < N; ++i) {
    cin >> queryType;
    cin >> numResults >> queryPosition[0] >> queryPosition[1];

    switch (queryType) {
      case 't':
        topicSet.clear();
        kdtree.kNNTopics(queryPosition);
        printSet(topicSet);
        break;
      case 'q':
        questionSet.clear();
        closestQuestionTopic.clear();
        kdtree.kNNQuestions(queryPosition);
        printSet(questionSet);
        break;
      default:
        break;
    }
  }
}

}  // namespace NearbySolver

int main() {
  NearbySolver::solve();
  return 0;
}
