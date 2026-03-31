
    // GLOBAL VARIABLES
    const personal_project_area = document.getElementById('personal-projects-area');




    // CODES
    async function loadProjects()
    {
        try
        {
            const response = await fetch("cpp-content/script-source/projects.json");
            if (!response.ok) throw new Error("The JSON file was not found!");

            const dados = await response.json();

            // Função que gera o HTML
            const createProjectHTML = (project) => `
                <div class="container-study"> 
                    <div> <h2 class="study">${project.title}</h2> </div> 
                    <div> <p>${project.text}</p> </div> 
                    <div> 
                        <div> 
                            <p> TECHNOLOGIES : ${project.tech} , </p> 
                            <p> PROGRAM SIZE : ${project.size} </p> 
                        </div> 
                    </div> 
                </div> 
                <div class="access-content"> 
                    <a href="${project.link}">Verify Project's Code</a>
                </div>`;


            // Percorre o array de projetos do JSON
            dados.project.forEach(projeto => {
                personal_project_area.innerHTML += createProjectHTML(projeto);
            });

        } catch (error) {
            console.error("Erro ao carregar o arquivo local:", error);
            personal_project_area.innerHTML += "<p>Erro ao carregar projetos.</p>";
        }
    }

    loadProjects();