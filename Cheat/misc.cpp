#include "misc.h"
#include <string>

void misc::testHookInGameFunction()
{
    /* 1 Versuch  (SEHR WICHTIGE INFOS: https://www.unknowncheats.me/forum/among-us/417887-dont-il2cpp-cheat.html)
    * - in scripts.json die Methode finden und diese dann hier definieren
    * - dann einfach ohne hook die methode aufrufen mit den entsprechenden parameter
    */
    /*[Token(Token = "0x6000978")]
      [Address(RVA = "0x501A20", Offset = "0x500620", VA = "0x180501A20", Slot = "34")]
      public override bool AddAmmo(int ammo)
      {
         return default(bool);
      }
    */
    /*
    *  RVA: 0x4FFA50 Offset: 0x4FE650 VA: 0x1804FFA50
    *  public int get_MagazineAmmo() { }
    */
    // try out to write mem with Inf Ammo offset
	printf_s("Test Output\n");
}