

// GLOBAL VARIABLES
    const personal_project_area = document.getElementById('personal-projects-area');

    // Replace these with your info
    const USER = 'leroperih';
    const REPO = 'cpp';

// CODES
    async function loadProjects() {

        // We fetch the contents of your specific folders from GitHub
        const url_directx = `https://api.github.com${USER}/${REPO}/cpp-content/code-source/ALL/directx`;
        const url_windows = `https://api.github.com${USER}/${REPO}/cpp-content/code-source/ALL/windows`;

        try {
            const [resDX, resWin] = await Promise.all([fetch(url_directx), fetch(url_windows)]);
            const dataDX = await resDX.json();
            const dataWin = await resWin.json();

            // Filter for items that are 'dir' (directories)
            const countDX = dataDX.filter(item => item.type === 'dir').length;
            const countWin = dataWin.filter(item => item.type === 'dir').length;

            const totalFolders = countDX + countWin;

            // Create the elements
            for (let i = 0; i < totalFolders; i++) {
                personal_project_area.innerHTML += "<p>Project Link Placeholder</p>";
            }
        } catch (error) {
            console.error("Error fetching from GitHub:", error);
        }
    }

    console.log("the javascript is working");