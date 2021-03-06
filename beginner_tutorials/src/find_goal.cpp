/*
 * find_goal.cpp
 *
 *  Created on: Apr 29, 2013
 *      Author: benedict
 */




#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <sensor_msgs/LaserScan.h>
#include "boost/thread/mutex.hpp"
#include "boost/thread/thread.hpp"
#include "ros/console.h"

class FindGoal
{
public:
  FindGoal();

private:
  void scanCallback(const sensor_msgs::LaserScan::ConstPtr& scan);
  void publish();
  void throughRow(const sensor_msgs::LaserScan::ConstPtr &scan);
  float calcFieldOfAttentionX(const sensor_msgs::LaserScan::ConstPtr &scan, float &searchRange, float &angleIncrement, int &numRanges, float &robotWidth, float &x);
  float calcFieldOfAttentionY(const sensor_msgs::LaserScan::ConstPtr &scan, float &searchRange, float &angleIncrement, int &numRanges, float &y, float &yr, float &yl);
  void decision(float &x, float &y, float &speed);
  void setVelocety(bool &turnDirection);


  double linear;
  double angular;

  ros::NodeHandle n;

  ros::Publisher vel_pub;
  ros::Subscriber scan_sub;

  ros::Timer timer,  timer1;


  geometry_msgs::Twist last_published;
};

FindGoal::FindGoal():
            linear(1),
            angular(0)
{
  vel_pub = n.advertise<geometry_msgs::Twist>("cmd_vel", 1);
  scan_sub = n.subscribe<sensor_msgs::LaserScan>("scan", 10, &FindGoal::scanCallback, this);

  timer = n.createTimer(ros::Duration(0.1), boost::bind(&FindGoal::publish, this));
}

void FindGoal::scanCallback(const sensor_msgs::LaserScan::ConstPtr &scan)
{
  this->throughRow(scan);
}

void FindGoal::throughRow(const sensor_msgs::LaserScan::ConstPtr &scan)
{
  // Durch Reihe navigieren


  int numRanges = scan->ranges.size();
  float angleIncrement = scan->angle_increment;

  float searchRange = 1.0;
  float robotWidth = 0.5;
  float speed = 1.0;
  bool turnDireciton = 0;


  float x = 0;
  float y = 0, yr = 0, yl = 0;

  x = this->calcFieldOfAttentionX(scan, searchRange, angleIncrement, numRanges, robotWidth, x);
  y = this->calcFieldOfAttentionY(scan, searchRange, angleIncrement, numRanges, y, yr, yl);

  printf("x: %1.3f, y: %1.3f\n",x,y);
 if (y!=0 && x > 0.3){
   this->decision(x, y ,speed);
 }
  if (y==0 && x > 0.3){
    //Wendemethode
  }


  this->setVelocety(turnDireciton);
}

float FindGoal::calcFieldOfAttentionX(const sensor_msgs::LaserScan::ConstPtr &scan, float &searchRange, float &angleIncrement, int &numRanges, float &robotWidth, float &x)
{
  float y = 0;
  int count = 0;

  for (int i = 0; i < numRanges; i++)
  {
    if (scan->ranges[i] < searchRange)
    {
      y = sin((i-(numRanges/2)) * angleIncrement) * scan->ranges[i];

      if (y > -robotWidth/2 && y < robotWidth/2)
      {
        x += cos((i-(numRanges/2)) * angleIncrement) * scan->ranges[i];
        count++;
      }
    }
  }

  if (count != 0)
  {
  x /= count;
  }
  else
  {
    x = 1;
  }
  return x;
}

float FindGoal::calcFieldOfAttentionY(const sensor_msgs::LaserScan::ConstPtr &scan, float &searchRange, float &angleIncrement, int &numRanges, float &y, float &yr, float &yl)
{
  int countr = 0, countl = 0;

  for (int i = 0; i < numRanges; i++)
  {
    if (scan->ranges[i] < searchRange)
    {
      if (i < numRanges/2)
      {
        yr += (sin((i-(numRanges/2)) * angleIncrement) * scan->ranges[i]);
        countr++;
      }
      else
      {
        yl += (sin((i-(numRanges/2)) * angleIncrement) * scan->ranges[i]);
        countl++;
      }
    }
  }

  if (countr > 0)
  {
    yr /= countr;
  }
  if(countl > 0)
  {
    yl /= countl;
  }

  y = yr + yl;

  return y;
}

void FindGoal::decision(float &x, float &y, float &speed)
{
  /*

  if(x < 0.3) // Floribot stoppt
  {
    linear = -speed;
  }
  else
  {
    linear = speed;
  }

  if (y < -0.1)
  {
    angular = -speed;
  }
  else if ((y >= -0.1 && y <= 0.1) && y!=0)
  {
    angular = 0.0;
  }
  else
  {
    angular = speed;
  }

  if (y == 0)
  {
    linear = 0.3;
  }
  */

  float minX = 0.3, minY = 0.1;

  if ((y >= -minY && y <= 0.1) && x > minX)      //Geradeaus Fahrt
  {
    linear = speed;
    angular = 0.0;
  }
  if (y < -minY && x > minX)                     //vorwärts rechts
  {
    linear = speed;
    angular = -speed;
  }
  if (y > minY && x > minX)                      //vortwärts links
  {
    linear = speed;
    angular = speed;
  }
  if (y > minY && x <= minX)                      //rückwärts links
  {
    linear = -speed;
    angular = speed;
  }
  if (y < -minY && x > minX)                      //rückwärts rechts
  {
    linear = -speed;
    angular = -speed;
  }
  if (y >= -0.07 && y <= 0.07 && x < minX)       //anhalten
  {
    linear = 0;
    angular = 0;
  }
}

void FindGoal::setVelocety(bool &turnDirection)
{
  geometry_msgs::Twist vel;

  vel.angular.z = angular;
  vel.linear.x = linear;

  last_published = vel;
}

void FindGoal::publish()
{
  vel_pub.publish(last_published);
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "find_goal");
  FindGoal find_goal;

  ros::spin();
}
