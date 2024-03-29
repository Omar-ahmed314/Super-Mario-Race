#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"

#include <iostream>

namespace our {

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json& config){
        // First, we store the window size for later use
        this->windowSize = windowSize;


        

        // Then we check if there is a sky texture in the configuration
        if(config.contains("sky")){
            // First, we create a sphere which will be used to draw the sky
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));
            
            // We can draw the sky using the same shader used to draw textured objects
            ShaderProgram* skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            skyShader->link();
            
            //TODO: (Req 9) Pick the correct pipeline state to draw the sky
            // Hints: the sky will be draw after the opaque objects so we would need depth testing but which depth funtion should we pick?
            // We will draw the sphere from the inside, so what options should we pick for the face culling.
            PipelineState skyPipelineState{};
            skyPipelineState.faceCulling.enabled = true;
            skyPipelineState.faceCulling.culledFace = GL_FRONT;
            skyPipelineState.depthTesting.enabled = true;
            skyPipelineState.depthTesting.function = GL_LEQUAL;

            // Load the sky texture (note that we don't need mipmaps since we want to avoid any unnecessary blurring while rendering the sky)
            std::string skyTextureFile = config.value<std::string>("sky", "");
            Texture2D* skyTexture = texture_utils::loadImage(skyTextureFile, false);

            // Setup a sampler for the sky 
            Sampler* skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Combine all the aforementioned objects (except the mesh) into a material 
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;
            
        }

        // Then we check if there is a postprocessing shader in the configuration
        if(config.contains("postprocess")){
            //TODO: (Req 10) Create a framebuffer
            glGenFramebuffers(1, &postprocessFrameBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postprocessFrameBuffer);
            // GL_DRAW_FRAMEBUFFER: draw to buffer
            // GL_READ_FRAMEBUFFER: read from buffer
            // GL_FRAMEBUFFER: read or draw


            //TODO: (Req 10) Create a color and a depth texture and attach them to the framebuffer
            
            // Hints: The color format can be (Red, Green, Blue and Alpha components with 8 bits for each channel).
            // The depth format can be (Depth component with 24 bits).
            // colorTarget = new our::Texture2D();      
            // colorTarget->bind();
            // int lvls = (int)glm::floor(glm::log2((float)glm::max(windowSize.x, windowSize.y))) + 1;
            // glTexStorage2D(GL_TEXTURE_2D, lvls, GL_RGBA8, windowSize.x, windowSize.y);        
            // glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
            // colorTarget->getOpenGLName(), 0);

            // depthTarget = new our::Texture2D();      
            // depthTarget->bind();
            // glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, windowSize.x, windowSize.y);        
            // glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
            // depthTarget->getOpenGLName(), 0);
            
            colorTarget=texture_utils::empty(GL_RGBA8, windowSize);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
            colorTarget->getOpenGLName(), 0);
            
            depthTarget=texture_utils::empty( GL_DEPTH_COMPONENT24, windowSize);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,  GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
            depthTarget->getOpenGLName(), 0);

            //TODO: (Req 10) Unbind the framebuffer just to be safe
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler* postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram* postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;
        }
        
    }

    void ForwardRenderer::destroy(){
        // Delete all objects related to the sky
        if(skyMaterial){
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
        }
        // Delete all objects related to post processing
        if(postprocessMaterial){
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
    }

    void ForwardRenderer::render(World* world){
        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent* camera = nullptr;
        opaqueCommands.clear();
        transparentCommands.clear();
        lightCommands.clear();
        for(auto entity : world->getEntities()){
            // If we hadn't found a camera yet, we look for a camera in this entity
            if(!camera) camera = entity->getComponent<CameraComponent>();

            if(auto lightRenderer = entity->getComponent<LightningComponent>();lightRenderer )
            {
                 LightCommand light;
                 light.type_light=lightRenderer->type;
                 light.diffuse=lightRenderer->diffuse;
                 light.color=lightRenderer->color;
                 light.cone_angles=lightRenderer->cone_angles;
                 light.direction=entity->localTransform.rotation;
                 light.position=entity->localTransform.position;
                 light.attenuation=lightRenderer->attenuation;
                 light.specular=lightRenderer->specular;

                 
                 lightCommands.push_back(light);

            }
            // If this entity has a mesh renderer component
            if(auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer){
                // We construct a command from it
                
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;
               
                // if it is transparent, we add it to the transparent commands list
                if(command.material->transparent){
                    transparentCommands.push_back(command);
                } else {
                // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }
        }

        // If there is no camera, we return (we cannot render without a camera)
        if(camera == nullptr) return;

        //TODO: (Req 8) Modify the following line such that "cameraForward" contains a vector pointing the camera forward direction
        // HINT: See how you wrote the CameraComponent::getViewMatrix, it should help you solve this one
        glm::vec3 cameraForward = camera->getViewMatrix()[2];
        std::sort(transparentCommands.begin(), transparentCommands.end(), [cameraForward](const RenderCommand& first, const RenderCommand& second){
            //TODO: (Req 8) Finish this function
            // HINT: the following return should return true "first" should be drawn before "second". 
            
            // First we have to calculate the distance between the camera center to the object center
            // so, we can do this using the dot product
            float cameraToFirstobjCenter = glm::dot(cameraForward, first.center);
            float cameraToSecondobjCenter = glm::dot(cameraForward, second.center);

            // Then return true if the first object is farest than the second
            return cameraToFirstobjCenter < cameraToSecondobjCenter;
        });

        //TODO: (Req 8) Get the camera ViewProjection matrix and store it in VP
        /// here we get the camera projection matrix multiplied by view matrix
        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        //TODO: (Req 8) Set the OpenGL viewport using windowSize
        /// set the viewport with the window aspect ratio
        glViewport(0, 0, windowSize[0], windowSize[1]);

        //TODO: (Req 8) Set the clear color to black and the clear depth to 1
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0);
        
        //TODO: (Req 8) Set the color mask to true and the depth mask to true (to ensure the glClear will affect the framebuffer)
        /// depth mask
        glDepthMask(true);
        /// depth colors mask
        glColorMask(true, true, true, true);
        

        // If there is a postprocess material, bind the framebuffer
        if(postprocessMaterial){
            //TODO: (Req 10) bind the framebuffer
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postprocessFrameBuffer);
        }

        //TODO: (Req 8) Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //TODO: (Req 8) Draw all the opaque commands
        // Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        /// This method loops over each command and draw it

        // Get the location of the transform uniform
        for(auto command : opaqueCommands)
        {      
             command.material->setup();
            if(!lightCommands.empty())
            {
              
         float angle = (float)glfwGetTime();

        glm::vec3 eye = glm::vec3(2*glm::sin(angle), 1, 2*glm::cos(angle)); //glm::vec3(0, 0, 2); //

       command.material->shader->set("eye",eye);
 
        command.material->shader->set("M", command.localToWorld);
       
        command.material->shader->set("VP", VP);
 
        command.material->shader->set("M_IT", glm::transpose(glm::inverse( command.localToWorld)));

           ShaderProgram *lightShader= command.material->shader;
           int light_size=lightCommands.size();
          lightShader->set("light_count",light_size);  
            for(int i=0; i < light_size;i++)


            { 
                  
command.material->shader->set("lights["+std::to_string(i)+"].position",lightCommands[i].position);               
command.material->shader->set("lights["+std::to_string(i)+"].direction",lightCommands[i].direction);               
command.material->shader->set("lights["+std::to_string(i)+"].type",lightCommands[i].type_light);
command.material->shader->set("lights["+std::to_string(i)+"].diffuse",lightCommands[i].diffuse);
command.material->shader->set("lights["+std::to_string(i)+"].specular",lightCommands[i].specular);
command.material->shader->set("lights["+std::to_string(i)+"].color",lightCommands[i].color);
command.material->shader->set("lights["+std::to_string(i)+"].attenuation",lightCommands[i].attenuation);
command.material->shader->set("lights["+std::to_string(i)+"].cone_angles",(glm::radians(lightCommands[i].cone_angles[0]), glm::radians(lightCommands[i].cone_angles[1])));





            }
        
        command.material->shader->set ("sky.top",{ 0.3, 0.6, 1.0});
        command.material->shader->set( "sky.middle",{ 0.3, 0.3, 0.3});
        command.material->shader->set("sky.bottom",{ 0.1, 0.1, 0.0});

         
        

            }
            //else
            {
 command.material->shader->set("transform", VP * command.localToWorld);
            }
          
           
            command.mesh->draw();
        }
        
        // If there is a sky material, draw the sky
        if(this->skyMaterial){
            //TODO: (Req 9) setup the sky material
            skyMaterial->setup();
            //TODO: (Req 9) Get the camera position
            glm::vec3 cameraPosition = camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1);
            //TODO: (Req 9) Create a model matrix for the sky such that it always follows the camera (sky sphere center = camera position)
            // Created the translation matrix that translate the sphere to the center of the camera
            glm::mat4 translationMatrix = glm::translate(
                glm::mat4(1.0f), 
                cameraPosition
                );
            glm::mat4 modelMatrix = translationMatrix /** scaleMatrix*/;
            //TODO: (Req 9) We want the sky to be drawn behind everything (in NDC space, z=1)
            // We can acheive this by multiplying by an extra matrix after the projection but what values should we put in it?

            // Here the third row third column will be modified into (0, 0, 0, 1) as Z index will be 1
            // 1000 x      x
            // 0100 y   =  y
            // 0001 z      1
            // 0001 1      1
            // x
            // y
            // z
            // 1
            glm::mat4 alwaysBehindTransform = glm::mat4(
            //  Row1, Row2, Row3, Row4
                1.0f, 0.0f, 0.0f, 0.0f, // Column1
                0.0f, 1.0f, 0.0f, 0.0f, // Column2
                0.0f, 0.0f, 0.0f, 0.0f, // Column3
                0.0f, 0.0f, 1.0f, 1.0f  // Column4
            );
            //TODO: (Req 9) set the "transform" uniform
            skyMaterial->shader->set("transform", alwaysBehindTransform * VP * modelMatrix);
            //TODO: (Req 9) draw the sky sphere
            skySphere->draw();
            
        }
        //TODO: (Req 8) Draw all the transparent commands
        // Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        /// This method loops over each command and draw it
        for(auto command : transparentCommands)
        {
            command.material->setup();
            command.material->shader->set("transform", VP * command.localToWorld);
            command.mesh->draw();
        }

        // If there is a postprocess material, apply postprocessing
        if(postprocessMaterial){
            //TODO: (Req 10) Return to the default framebuffer
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);       
            //TODO: (Req 10) Setup the postprocess material and draw the fullscreen triangle
            postprocessMaterial->setup();
            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES,0,3);

        }
    }

}
