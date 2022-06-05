#include "material.hpp"

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"
#include<iostream>
namespace our {

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const {
        //TODO: (Req 6) Write this function
        pipelineState.setup(); //it calls the setup function we did before (of the pipeline struct)
        shader->use(); //this calls the function that set the program shader to be used
        
    }

    // This function read the material data from a json object
    void Material::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;

        if(data.contains("pipelineState")){
            pipelineState.deserialize(data["pipelineState"]);
        }
        shader = AssetLoader<ShaderProgram>::get(data["shader"].get<std::string>());
        transparent = data.value("transparent", false);
    }

    // This function should call the setup of its parent and
    // set the "tint" uniform to the value in the member variable tint 
    void TintedMaterial::setup() const {
        //TODO: (Req 6) Write this function
        Material::setup(); //it calls the material setup from above then..
        shader->set("tint",tint); //it sends the 'tint' value to the uniform in the shader
                                  //1st param is the name of the uniform
                                  //2nd param is the value to be set to it (which is the member tint)
    }

    // This function read the material data from a json object
    void TintedMaterial::deserialize(const nlohmann::json& data){
        Material::deserialize(data);
        if(!data.is_object()) return;
        tint = data.value("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // This function should call the setup of its parent and
    // set the "alphaThreshold" uniform to the value in the member variable alphaThreshold
    // Then it should bind the texture and sampler to a texture unit and send the unit number to the uniform variable "tex" 
    void TexturedMaterial::setup() const {
        //TODO: (Req 6) Write this function
        TintedMaterial::setup(); //we call the tintedmaterial's setup that we just did
        shader->set("alphaThreshold",alphaThreshold); //we set the uniform alphathreshold to the value of the member alphaThreshold 
        glActiveTexture(GL_TEXTURE0); // we select an active texture unit.
        texture->bind(); // it calls a function to bind the texture to the texture unit
        sampler->bind(0); // it calls a function to bind the sampler to the texture unit
                          // it takes a paramater which is the number of the active texture unit
        shader->set("tex",0); // it calls a function to set the uniform tex with the unit number
                              //1st param is the name of the uniform
                              //2nd param is the value to be set to it (which is the unit number)
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json& data){
        TintedMaterial::deserialize(data);
        if(!data.is_object()) return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
    }




      void LitMaterial::setup() const {

          Material::setup(); //material setup
           
          glActiveTexture(GL_TEXTURE0);
          albedo->bind();
          sampler->bind(0);
          shader->set("material.albedo",0);

         glActiveTexture(GL_TEXTURE1);
         specular->bind();
         sampler->bind(1);
         shader->set("material.specular",1);

         glActiveTexture(GL_TEXTURE2);
         ambient_occlusion->bind();
         sampler->bind(2);
         shader->set("material.ambient_occlusion",2);
         
        glActiveTexture(GL_TEXTURE3);
        roughness->bind();
        sampler->bind(3);
        shader->set("material.roughness",3);

        glActiveTexture(GL_TEXTURE4);
        emissive->bind(); 
        sampler->bind(4);
        shader->set("material.emissive",4);

         
       
        
         
        
    }

    // This function read the material data from a json object
    void LitMaterial::deserialize(const nlohmann::json& data){
         

        Material::deserialize(data);
        if(!data.is_object()) return;

 
             
            if(data.contains("albedo"))
           { 
             albedo=AssetLoader<Texture2D>::get(data.value("albedo", ""));
           }       
            
            
             if(data.contains("specular"))
            {specular=AssetLoader<Texture2D>::get(data.value("specular", ""));
            
            }
             if(data.contains("ambient_occlusion"))
           
           {
                ambient_occlusion=AssetLoader<Texture2D>::get(data.value("ambient_occlusion", ""));
           
           }
            if(data.contains("roughness"))
          {
                roughness=AssetLoader<Texture2D>::get(data.value("roughness", ""));
                std::cout<<"roughness\n";
          }
             if(data.contains("emissive"))
            {
               
                emissive=AssetLoader<Texture2D>::get(data.value("emissive", ""));
            }
           sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
      }

}