#include "game/Player.h"
#include "game/StreetEnemy.h"
#include <cassert>

using namespace district_fury;

int main() {
    CombatBox a{0, 0, 10, 10};
    CombatBox b{5, 5, 10, 10};
    CombatBox c{20, 20, 5, 5};
    assert(a.Intersects(b));
    assert(!a.Intersects(c));

    Player player;
    assert(player.hp == player.maxHp);
    player.TakeDamage(20);
    assert(player.hp == 80);
    assert(player.rage >= 15);

    player.state = PlayerState::Attack;
    player.attackType = AttackType::Punch;
    player.attackElapsed = 0.15f;
    assert(player.AttackIsActive());

    StreetEnemy enemy;
    enemy.Init({100, 575, 0}, StreetEnemyType::Brute);
    enemy.active = true;
    const int originalHp = enemy.hp;
    enemy.TakeDamage(20, {0, 0, 0});
    assert(enemy.hp == originalHp - 20);
    enemy.TakeDamage(10000, {0, 0, 0});
    assert(enemy.IsDefeated());
    return 0;
}
