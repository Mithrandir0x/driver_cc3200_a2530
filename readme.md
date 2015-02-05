
## what's this?

This is a bunch of projects to allow communication between the [Anaren Air Module's A2530E24x](https://www.anaren.com/air/products/air-for-zigbee-standard-apps) (you can look at [this prospect](https://www.anaren.com/sites/default/files/Part-Datasheets/A2530E24A-LPZ_Product-Brief.pdf) for more info about the thingie) and the [Texas Instruments CC3200XL LaunchPad](http://www.ti.com/tool/cc3200-launchxl).

## why this?

Obviously, why not?

As I haven't found any implementation available that integrated that radio device with the launchpad, and, after all, it was an assignment, I found it rather fun to play with it.

## How?

Though it is possible to plug in the CC3200 board the Anaren's shield, there's a little soldering to be done before trying to use it.

### Hardware changes on the CC3200

...

### Software

Anaren supplies a [BoosterPack CD](https://www.anaren.com/sites/default/files/uploads/File/A2530-ZigBee-BoosterPack-CD.zip) where it is possible to grab the code and start messing things around. Also, their [Wiki](https://www.anaren.com/air-wiki-zigbee/Main_Page) is quite helpful to understand how their hardware works.

From Anaren's firmware code, the only relevant part is the [HAL](https://github.com/Mithrandir0x/driver_cc3200_a2530/tree/master/a2530_firmware/HAL) folder, which contains the integration between both hardwares. As you will see, it's pretty straightforward and depends on the driverlib project from the CC3200 SDK, and I skimmed up all the various HAL implementations supplied in the Booster Pack for simplicity's sake.

I think it is possible to import the `a2530_firmware` project to another workspace, and be made as a dependency without any problem.

> The only requirement is to point to the CC3200 SDK's path in the project's **Linked Resources** variable `CC3200_SDK_ROOT`.


