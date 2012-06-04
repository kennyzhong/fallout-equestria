#ifndef  MAPELEMENT_HPP
# define MAPELEMENT_HPP

# include <list>
# include "pathfinding.hpp"

/*class MapElement
{
public:
  struct Position
  {
    Position(void)         : x(0), y(0) {}
    Position(int x, int y) : x(x), y(y) {}
    int x;
    int y;

    int get_x(void) const { return (x);  }
    int get_y(void) const { return (y);  }
    void set_x(int x)     { this->x = x; }
    void set_y(int y)     { this->y = y; }
  };
  
  MapElement(void)
  {
    _collisionEnabled = true;
    OccupyPosition(0, 0);
  }

  Position GetPosition(void) const { return (_mapPos); }

  void     OccupyPosition(int x, int y)
  {
    _occupedNodes.push_back(Position(x, y));
  }

  void     UnocccupyPosition(int x, int y);

  void     SetCollisionEnabled(bool enable) { _collisionEnabled = enable; }

  virtual void    ProcessCollision(Pathfinding* map);
  void            UnprocessCollision(Pathfinding* map);

protected:
  Position         _mapPos;

  void            WithdrawArc(Pathfinding::Node& node, Pathfinding::Node::Arc arc);

private:
  typedef std::list<Position> Positions;

  Positions        _occupedNodes;
  bool             _collisionEnabled;

  struct WithdrawedArc
  {
    Pathfinding::Node*                from;
    Pathfinding::Node*                to;
    float                             weigth;
    Pathfinding::Node::Arc::Observer* observer;
  };

  typedef std::list<WithdrawedArc> WithdrawedArcs;

  WithdrawedArcs _withdrawedArcs;
};*/

#endif