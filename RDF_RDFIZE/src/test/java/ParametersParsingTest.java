import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.RETURNS_SMART_NULLS;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import net.antidot.paf.Configuration;
import net.antidot.paf.Handle;
import net.antidot.protobuf.event.EventProtos.Level;
import net.antidot.protobuf.event.EventProtos.Type;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

/**
 * @author lm
 *
 */
public class ParametersParsingTest {

    /**
     * @throws java.lang.Exception
     */
    @Before
    public void setUp() throws Exception {
    }

    /**
     * @throws java.lang.Exception
     */
    @After
    public void tearDown() throws Exception {
    }
    
    /**
     * Test method for {@link Db2TripleFilter#init()}.
     */
    @Test
    public void testDirectMappingMinimal() throws Exception {	
	Configuration mockConf = mock(Configuration.class, RETURNS_SMART_NULLS);
	when(mockConf.has_arg(anyString())).thenReturn(true);
	when(mockConf.get_string("rdb2rdf_mode")).thenReturn("DirectMapping");
	when(mockConf.get_string("driver")).thenReturn("com.mysql.jdbc.Driver");
	
	Handle mockHandle = mock(Handle.class);
	final Answer<Object> boLogToStderr = new Answer<Object>() {
	        public Object answer(InvocationOnMock invocation) {
	            Object[] args = invocation.getArguments();
	            System.err.println("LOG to BO: " + args[1]);
	            return null;
	        }};
	doAnswer(boLogToStderr).when(mockHandle).log(any(Type.class), anyString(), any(Level.class), anyString());
	doAnswer(boLogToStderr).when(mockHandle).log(any(Type.class), anyString(), any(Level.class));	

	Db2TripleFilter filter = new Db2TripleFilter(mockConf, mockHandle);
	filter.init(); // May not throw anything
	
	// Verify all mandatory arguments have been read
	verify(mockConf).get_string("rdb2rdf_mode");
	verify(mockConf).get_string("driver");
	verify(mockConf).get_string("login");
	verify(mockConf).get_string("password");
	verify(mockConf).get_string("url");
	
	// Verify all optional arguments have been cheched
	verify(mockConf).has_arg("base_uri");
	verify(mockConf).has_arg("create_doc_with_base_uri");
	verify(mockConf).has_arg("nativestore_path");
    }

    /**
     * Test method for {@link Db2TripleFilter#init()}.
     */
    @Test
    public void testR2RMLMinimal() throws Exception {
	Configuration mockConf = mock(Configuration.class, RETURNS_SMART_NULLS);
	when(mockConf.has_arg(anyString())).thenReturn(true);
	when(mockConf.get_string("rdb2rdf_mode")).thenReturn("R2RML");
	when(mockConf.get_string("driver")).thenReturn("com.mysql.jdbc.Driver");
	
	Handle mockHandle = mock(Handle.class);

	Db2TripleFilter filter = new Db2TripleFilter(mockConf, mockHandle);
	filter.init(); // May not throw anything

	// Verify all mandatory arguments have been read
	verify(mockConf).get_string("rdb2rdf_mode");
	verify(mockConf).get_string("driver");
	verify(mockConf).get_string("login");
	verify(mockConf).get_string("password");
	verify(mockConf).get_string("url");

	// R2RML mandatory args
	verify(mockConf).get_string("r2rml_file");

	// Verify all optional arguments have been cheched
	verify(mockConf).has_arg("base_uri");
	verify(mockConf).has_arg("create_doc_with_base_uri");
	verify(mockConf).has_arg("nativestore_path");	
    }
    
}
