// Offscreen proof using the native ReSTIR/Vulkan renderer, not SoftwarePresentation's rasterizer.
#include "Frontier/Game/RestirPresentation.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
int main(int argc,char** argv)
{
    const char* Out = argc>1 ? argv[1] : "restir.ppm";
    const char* Content = argc>2 ? argv[2] : "EngineContent";
    Frontier::RestirPresentation Renderer;
    std::string Diagnostic;
    if (!Renderer.Initialize(Content, 288, 162, Diagnostic)) { std::cerr<<Diagnostic<<'\n'; return 1; }
    auto Position=Renderer.VehiclePosition();
    std::cout << "Vehicle archive position: " << Position[0] << ' ' << Position[1] << ' ' << Position[2] << '\n';
    Frontier::PresentationCamera Camera; Camera.FocusMetres=Position; Camera.FocusMetres[2]+=0.6f;
    Frontier::PresentationFrame Frame;
    for (int I=0;I<12;++I) {
        if (argc>3 && std::string(argv[3])=="--move" && I==6) Renderer.SetVehiclePose({3.0f,0.0f,0.53f},30.0f);
        if (!Renderer.Render(Camera,1.0/30.0,Frame,Diagnostic)) { std::cerr<<Diagnostic<<'\n'; return 1; }
        std::cout << "ReSTIR frame " << I+1 << "\n" << std::flush;
    }
    std::ofstream File(Out,std::ios::binary); File<<"P6\n"<<Frame.Width<<' '<<Frame.Height<<"\n255\n";
    File.write(reinterpret_cast<const char*>(Frame.Rgb.data()),Frame.Rgb.size());
    return File ? 0 : 1;
}
